import tkinter as tk
from tkinter import messagebox, Listbox, Scrollbar
import websocket
import threading
import json
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from datetime import datetime

class RingBuffer:
    def __init__(self):
        self.size = 1000
        self.values = [None] * self.size
        self.timestamps = [None] * self.size
        self.index = 0

    def push_front(self, value, timestamp):
        self.values[self.index] = value
        self.timestamps[self.index] = timestamp
        self.index = (self.index + 1) % self.size

    def get_data(self):
        values = self.values[self.index:] + self.values[:self.index]
        timestamps = self.timestamps[self.index:] + self.timestamps[:self.index]
        return values, timestamps

    def get_front(self, count=1):
        if count > self.size:
            count = self.size
        start_index = (self.index - count) % self.size
        if start_index < self.index:
            return list(zip(self.timestamps[start_index:self.index], self.values[start_index:self.index]))
        else:
            return list(zip(self.timestamps[start_index:], self.values[start_index:])) + \
                   list(zip(self.timestamps[:self.index], self.values[:self.index]))

class WebSocketApp:
    def __init__(self, master):
        self.master = master
        self.master.title("WebSocket Client")
        self.master.geometry("800x600")
        
        self.uri_label = tk.Label(master, text="WebSocket URI:")
        self.uri_label.pack(pady=10)
        
        uri_frame = tk.Frame(master)
        uri_frame.pack(pady=10)

        self.uri_entry_1 = tk.Entry(uri_frame, width=35)
        self.uri_entry_1.pack(side=tk.LEFT)
        self.uri_entry_1.insert(0, "ws://localhost:8765/wsi")  # Server 1 URI

        self.uri_entry_2 = tk.Entry(uri_frame, width=35)
        self.uri_entry_2.pack(side=tk.LEFT, padx=10)
        self.uri_entry_2.insert(0, "ws://localhost:8766/wsi")  # Server 2 URI

        self.connection_status_label_1 = tk.Label(uri_frame, text="", font=("Arial", 12), fg="green")
        self.connection_status_label_1.pack(side=tk.LEFT, padx=(10, 0))

        self.connection_status_label_2 = tk.Label(uri_frame, text="", font=("Arial", 12), fg="green")
        self.connection_status_label_2.pack(side=tk.LEFT, padx=(10, 0))

        button_frame = tk.Frame(master)
        button_frame.pack(pady=10)

        self.start_stop_button = tk.Button(button_frame, text="Start", command=self.toggle_connection, width=20, height=2)
        self.start_stop_button.grid(row=0, column=0, padx=10)

        self.show_data_button = tk.Button(button_frame, text="Show Last 100 Data Points", command=self.show_last_data, width=20, height=2)
        self.show_data_button.grid(row=0, column=1, padx=10)

        self.show_latest_data_button = tk.Button(button_frame, text="Show Latest Data Point", command=self.show_latest_data, width=20, height=2)
        self.show_latest_data_button.grid(row=0, column=2, padx=10)

        self.latest_data_label = tk.Label(master, text="", font=("Arial", 12))
        self.latest_data_label.pack(pady=10)

        value_frame = tk.Frame(master)
        value_frame.pack(pady=10)

        # Server 1 Min/Max values
        self.value_min_label_1 = tk.Label(value_frame, text="Server 1 Value Min:")
        self.value_min_label_1.grid(row=0, column=0)

        self.value_min_entry_1 = tk.Entry(value_frame, width=15)
        self.value_min_entry_1.grid(row=0, column=1)

        self.value_max_label_1 = tk.Label(value_frame, text="Server 1 Value Max:")
        self.value_max_label_1.grid(row=0, column=2)

        self.value_max_entry_1 = tk.Entry(value_frame, width=15)
        self.value_max_entry_1.grid(row=0, column=3)

        # Server 2 Min/Max values
        self.value_min_label_2 = tk.Label(value_frame, text="Server 2 Value Min:")
        self.value_min_label_2.grid(row=1, column=0)

        self.value_min_entry_2 = tk.Entry(value_frame, width=15)
        self.value_min_entry_2.grid(row=1, column=1)

        self.value_max_label_2 = tk.Label(value_frame, text="Server 2 Value Max:")
        self.value_max_label_2.grid(row=1, column=2)

        self.value_max_entry_2 = tk.Entry(value_frame, width=15)
        self.value_max_entry_2.grid(row=1, column=3)

        self.send_values_button = tk.Button(master, text="Send Values", command=self.send_min_max, width=20, height=2)
        self.send_values_button.pack(pady=10)

        self.figure, self.ax = plt.subplots(figsize=(8, 5))
        self.canvas = FigureCanvasTkAgg(self.figure, master)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

        self.ring_buffer_1 = RingBuffer()  # For Server 1
        self.ring_buffer_2 = RingBuffer()  # For Server 2
        self.update_counter = 0

        self.connected_1 = False
        self.connected_2 = False
        self.thread_1 = None
        self.thread_2 = None
        self.running_1 = False
        self.running_2 = False
    def on_message_1(self, ws, message):
        try:
            data = json.loads(message)
            timestamp, value = data['timestamp'], data['value']
            dt = datetime.fromisoformat(timestamp)
            timestamp_unix = dt.timestamp()

            self.ring_buffer_1.push_front(value, timestamp_unix)
            self.update_counter += 1

        except (json.JSONDecodeError, KeyError):
            print("Fehler beim Verarbeiten der Nachricht von Server 1:", message)

    def on_message_2(self, ws, message):
        try:
            data = json.loads(message)
            timestamp, value = data['timestamp'], data['value']
            dt = datetime.fromisoformat(timestamp)
            timestamp_unix = dt.timestamp()

            self.ring_buffer_2.push_front(value, timestamp_unix)
            self.update_counter += 1

        except (json.JSONDecodeError, KeyError):
            print("Fehler beim Verarbeiten der Nachricht von Server 2:", message)

    def on_error(self, ws, error):
        print("WebSocket Fehler:", error)

    def on_close_1(self, ws):
        print("WebSocket Verbindung zu Server 1 geschlossen")
        self.connected_1 = False
        self.running_1 = False
        self.connection_status_label_1.config(text="")

    def on_close_2(self, ws):
        print("WebSocket Verbindung zu Server 2 geschlossen")
        self.connected_2 = False
        self.running_2 = False
        self.connection_status_label_2.config(text="")

    def on_open_1(self, ws):
        print("WebSocket Verbindung zu Server 1 hergestellt")
        self.connected_1 = True
        self.running_1 = True
        self.connection_status_label_1.config(text="✔️ Verbunden")
        self.value_min_entry_1.config(state=tk.NORMAL)
        self.value_max_entry_1.config(state=tk.NORMAL)

    def on_open_2(self, ws):
        print("WebSocket Verbindung zu Server 2 hergestellt")
        self.connected_2 = True
        self.running_2 = True
        self.connection_status_label_2.config(text="✔️ Verbunden")
        self.value_min_entry_2.config(state=tk.NORMAL)
        self.value_max_entry_2.config(state=tk.NORMAL)

    def run_1(self):
        uri = self.uri_entry_1.get()
        websocket.enableTrace(False)
        ws = websocket.WebSocketApp(uri,
                                    on_message=self.on_message_1,
                                    on_error=self.on_error,
                                    on_close=self.on_close_1)
        ws.on_open = self.on_open_1
        self.ws_1 = ws
        ws.run_forever()

    def run_2(self):
        uri = self.uri_entry_2.get()
        websocket.enableTrace(False)
        ws = websocket.WebSocketApp(uri,
                                    on_message=self.on_message_2,
                                    on_error=self.on_error,
                                    on_close=self.on_close_2)
        ws.on_open = self.on_open_2
        self.ws_2 = ws
        ws.run_forever()

    def toggle_connection(self):
        if self.connected_1 and self.connected_2:
            self.start_stop_button.config(text="Start")
            self.connected_1 = self.connected_2 = False
            self.running_1 = self.running_2 = False
            if self.ws_1:
                self.ws_1.close()
            if self.ws_2:
                self.ws_2.close()
        else:
            self.start_stop_button.config(text="Stop")
            self.running_1 = self.running_2 = True
            self.thread_1 = threading.Thread(target=self.run_1)
            self.thread_1.start()
            self.thread_2 = threading.Thread(target=self.run_2)
            self.thread_2.start()

            self.master.after(20, self.update_plot)

    def update_plot(self):
        self.ax.clear()

        values_1, timestamps_1 = self.ring_buffer_1.get_data()
        values_2, timestamps_2 = self.ring_buffer_2.get_data()

        valid_indices_1 = [i for i, val in enumerate(values_1) if val is not None]
        valid_values_1 = [values_1[i] for i in valid_indices_1]
        valid_timestamps_1 = [timestamps_1[i] for i in valid_indices_1]

        valid_indices_2 = [i for i, val in enumerate(values_2) if val is not None]
        valid_values_2 = [values_2[i] for i in valid_indices_2]
        valid_timestamps_2 = [timestamps_2[i] for i in valid_indices_2]

        if valid_timestamps_1:
            self.ax.plot(valid_timestamps_1, valid_values_1, color='red', label="Server 1")
        if valid_timestamps_2:
            self.ax.plot(valid_timestamps_2, valid_values_2, color='blue', label="Server 2")
        
        self.ax.set_title("Empfangene Werte")
        self.ax.set_xlabel("Zeitstempel")
        self.ax.set_ylabel("Wert")
        self.ax.legend()

        self.ax.xaxis.set_major_formatter(plt.FuncFormatter(self.format_unix_timestamps))
        self.canvas.draw()

        self.master.after(20, self.update_plot)

    def format_unix_timestamps(self, x, pos):
        return datetime.fromtimestamp(x).strftime("%Y-%m-%d %H:%M:%S")

    def send_min_max(self):
        if self.connected_1 or self.connected_2:
            try:
                value_min_1 = int(self.value_min_entry_1.get())
                value_max_1 = int(self.value_max_entry_1.get())

                value_min_2 = int(self.value_min_entry_2.get())
                value_max_2 = int(self.value_max_entry_2.get())

                message_1 = json.dumps({
                    "Value_min": value_min_1,
                    "Value_max": value_max_1
                })

                message_2 = json.dumps({
                    "Value_min": value_min_2,
                    "Value_max": value_max_2
                })

                if self.connected_1:
                    self.ws_1.send(message_1)
                if self.connected_2:
                    self.ws_2.send(message_2)

                print(f"Gesendete Werte Server 1: Value_min = {value_min_1}, Value_max = {value_max_1}")
                print(f"Gesendete Werte Server 2: Value_min = {value_min_2}, Value_max = {value_max_2}")
            except ValueError:
                messagebox.showerror("Fehler", "Bitte geben Sie gültige Ganzzahlen für Value Min und Value Max ein.")

    def show_last_data(self):
        data_1 = self.ring_buffer_1.get_front(100)
        data_2 = self.ring_buffer_2.get_front(100)

        top = tk.Toplevel(self.master)
        top.title("Letzte 100 Datenpunkte")
        top.geometry("500x400")
        top.resizable(True, True)

        scrollbar = Scrollbar(top)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        listbox = Listbox(top, width=80, height=20, yscrollcommand=scrollbar.set)
        listbox.pack(fill=tk.BOTH, expand=True)

        scrollbar.config(command=listbox.yview)

        listbox.insert(tk.END, "Server 1:")
        for timestamp, value in data_1:
            if timestamp and value:
                formatted_time = datetime.fromtimestamp(timestamp).strftime("%Y-%m-%d %H:%M:%S")
                listbox.insert(tk.END, f"{formatted_time} - {value}")

        listbox.insert(tk.END, "\nServer 2:")
        for timestamp, value in data_2:
            if timestamp and value:
                formatted_time = datetime.fromtimestamp(timestamp).strftime("%Y-%m-%d %H:%M:%S")
                listbox.insert(tk.END, f"{formatted_time} - {value}")

    def show_latest_data(self):
        latest_data_1 = self.ring_buffer_1.get_front(1)
        latest_data_2 = self.ring_buffer_2.get_front(1)

        if latest_data_1:
            timestamp, value = latest_data_1[-1]
            formatted_time = datetime.fromtimestamp(timestamp).strftime("%Y-%m-%d %H:%M:%S")
            self.latest_data_label.config(text=f"Letzter Wert von Server 1: {formatted_time} - {value}")

        if latest_data_2:
            timestamp, value = latest_data_2[-1]
            formatted_time = datetime.fromtimestamp(timestamp).strftime("%Y-%m-%d %H:%M:%S")
            self.latest_data_label.config(text=f"Letzter Wert von Server 2: {formatted_time} - {value}")

if __name__ == "__main__":
    root = tk.Tk()
    app = WebSocketApp(root)
    root.mainloop()

