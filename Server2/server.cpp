#include "include/json.hpp"
#include <crow.h>
#include <chrono>
#include <thread>
#include <iomanip>
#include <iostream>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <cmath>

using json = nlohmann::json;
using namespace std::chrono_literals;

struct ClientSession {
    crow::websocket::connection* connection;
    std::atomic<double> frequency;
    std::atomic<double> value_min;
    std::atomic<double> value_max;
    std::atomic<bool> active;

    ClientSession(crow::websocket::connection* conn, double freq, double min_val, double max_val)
        : connection(conn), frequency(freq), value_min(min_val), value_max(max_val), active(true) {}
};

// Funktion für den gemeinsamen Sinuskurven-Thread (für alle Clients auf /wss)
void shared_sinus_message_sender(std::vector<crow::websocket::connection*>& connections, 
                                 std::atomic<bool>& active, 
                                 std::mutex& conn_mutex, 
                                 double& frequency, 
                                 double& value_min, 
                                 double& value_max) {
    double time = 0.0;

    while (active) {
        std::this_thread::sleep_for(7ms);

        // Sinuswert berechnen
        double amplitude = (value_max - value_min) / 2.0;
        double offset = (value_max + value_min) / 2.0;
        double value = amplitude * std::sin(frequency * time) + offset;
        time += 0.01;

        // Zeitstempel erstellen
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%dT%H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();

        // JSON-Nachricht erstellen
        json message = {
            {"timestamp", ss.str()},
            {"value", value}
        };

        // Nachricht senden
        std::string message_str = message.dump();
        std::lock_guard<std::mutex> lock(conn_mutex);
        for (auto conn_ptr : connections) {
            try {
                conn_ptr->send_text(message_str);
                std::cout << "Gesendete gemeinsame Nachricht: " << message_str << " an Client: " << conn_ptr << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Fehler beim Senden an Client " << conn_ptr << ": " << e.what() << std::endl;
                active = false;
            }
        }
    }

    std::cout << "shared_sinus_message_sender Thread beendet." << std::endl;
}

// Funktion für individuelle Sinuskurven-Threads (für jeden Client auf /wsi)
void individual_sinus_message_sender(ClientSession* session) {
    double time = 0.0;

    while (session->active) {
        std::this_thread::sleep_for(7ms);

        // Sinuswert mit individuellen Einstellungen berechnen
        double amplitude = (session->value_max - session->value_min) / 2.0;
        double offset = (session->value_max + session->value_min) / 2.0;
        double value = amplitude * std::sin(session->frequency * time) + offset;
        time += 0.01;

        // Zeitstempel erstellen
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%dT%H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();

        // JSON-Nachricht erstellen
        json message = {
            {"timestamp", ss.str()},
            {"value", value}
        };

        // Nachricht senden
        std::string message_str = message.dump();
        try {
            session->connection->send_text(message_str);
            std::cout << "Gesendete individuelle Nachricht: " << message_str << " an Client: " << session->connection << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Fehler beim Senden an individuellen Client " << session->connection << ": " << e.what() << std::endl;
            session->active = false;
        }
    }

    std::cout << "Individueller sinus_message_sender Thread für Client beendet: " << session->connection << std::endl;
}

int main() {
    crow::SimpleApp app;

    // Einstellungen für den gemeinsamen Sinus
    double frequency = 1.0;
    double value_min = -1.0;
    double value_max = 1.0;
    std::vector<crow::websocket::connection*> connections;
    std::mutex conn_mutex;
    auto active = std::make_shared<std::atomic<bool>>(true);

    // Sammlung für individuelle Sessions
    std::unordered_map<crow::websocket::connection*, std::unique_ptr<ClientSession>> individual_sessions;
    std::mutex session_mutex;

    // Starten des shared_sinus_message_sender Threads für die /wss Route
    std::thread([&connections, active, &conn_mutex, &frequency, &value_min, &value_max]() {
        shared_sinus_message_sender(connections, *active, conn_mutex, frequency, value_min, value_max);
    }).detach();

    // /wss Route für gemeinsame Sinuskurven
    CROW_WEBSOCKET_ROUTE(app, "/ws")
    .onopen([&connections, &conn_mutex](crow::websocket::connection& conn) {
        std::cout << "Gemeinsame WebSocket-Verbindung geöffnet für Sinuskurve (Client): " << &conn << std::endl;
        std::lock_guard<std::mutex> lock(conn_mutex);
        connections.push_back(&conn);
    })
    .onmessage([&frequency, &value_min, &value_max](crow::websocket::connection& conn, const std::string& message, bool is_binary) {
        std::cout << "Nachricht vom gemeinsamen Client (Sinuskurve) empfangen: " << message << std::endl;
        json jsonData;
        try {
            jsonData = json::parse(message);
            if (jsonData.contains("Frequency")) {
                frequency = jsonData["Frequency"];
                std::cout << "Empfangene gemeinsame Frequenz: " << frequency << std::endl;
            }
            if (jsonData.contains("Value_min") && jsonData.contains("Value_max")) {
                value_min = jsonData["Value_min"];
                value_max = jsonData["Value_max"];
                std::cout << "Empfangene gemeinsame Werte: Value_min = " << value_min << ", Value_max = " << value_max << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Fehler beim Parsen der Nachricht: " << e.what() << std::endl;
        }
    })
    .onclose([&connections, &conn_mutex](crow::websocket::connection& conn, const std::string& reason) {
        std::cout << "Gemeinsame WebSocket-Verbindung geschlossen für Client (Sinuskurve): " << &conn << " Grund: " << reason << std::endl;
        std::lock_guard<std::mutex> lock(conn_mutex);
        connections.erase(std::remove(connections.begin(), connections.end(), &conn), connections.end());
    });

    // /wsi Route für individuelle Sinuskurven
    CROW_WEBSOCKET_ROUTE(app, "/wsi")
    .onopen([&individual_sessions, &session_mutex](crow::websocket::connection& conn) {
        std::cout << "Individuelle WebSocket-Verbindung geöffnet für Sinuskurve (Client): " << &conn << std::endl;
        
        // Standardwerte für individuelle Session
        auto session = std::make_unique<ClientSession>(&conn, 1.0, -1.0, 1.0);

        // Session zur Sammlung hinzufügen
        {
            std::lock_guard<std::mutex> lock(session_mutex);
            individual_sessions[&conn] = std::move(session);
        }

        // Thread für individuellen sinus_message_sender starten
        std::thread(individual_sinus_message_sender, individual_sessions[&conn].get()).detach();
    })
    .onmessage([&individual_sessions, &session_mutex](crow::websocket::connection& conn, const std::string& message, bool is_binary) {
        std::cout << "Nachricht vom individuellen Client (Sinuskurve) empfangen: " << message << std::endl;

        json jsonData;
        try {
            jsonData = json::parse(message);
            std::lock_guard<std::mutex> lock(session_mutex);

            auto it = individual_sessions.find(&conn);
            if (it != individual_sessions.end()) {
                auto& session = it->second;

                // Einstellungen für diesen Client aktualisieren
                if (jsonData.contains("Frequency")) {
                    session->frequency = jsonData["Frequency"];
                    std::cout << "Empfangene Frequenz (individueller Client): " << session->frequency << std::endl;
                }
                if (jsonData.contains("Value_min") && jsonData.contains("Value_max")) {
                    session->value_min = jsonData["Value_min"];
                    session->value_max = jsonData["Value_max"];
                    std::cout << "Empfangene Werte für Sinuskurve (individueller Client): Value_min = " << session->value_min 
                              << ", Value_max = " << session->value_max << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Fehler beim Parsen der Nachricht: " << e.what() << std::endl;
        }
    })
    .onclose([&individual_sessions, &session_mutex](crow::websocket::connection& conn, const std::string& reason) {
        std::cout << "Individuelle WebSocket-Verbindung geschlossen für Client (Sinuskurve): " << &conn << " Grund: " << reason << std::endl;
        
        std::lock_guard<std::mutex> lock(session_mutex);
        auto it = individual_sessions.find(&conn);
        if (it != individual_sessions.end()) {
            it->second->active = false;  // Beendet den Sende-Thread
            individual_sessions.erase(it);
        }
    });

    // Starte den Server auf Port 8766
    app.port(8766).run();
}

