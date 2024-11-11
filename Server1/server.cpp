#include "include/json.hpp"
#include <crow.h>
#include <chrono>
#include <thread>
#include <iomanip>
#include <iostream>
#include <atomic>
#include <mutex>
#include <vector>
#include <unordered_map>

using json = nlohmann::json;
using namespace std::chrono_literals;

struct ClientSession {
    crow::websocket::connection* connection;
    std::atomic<int> value;
    int value_min;
    int value_max;
    std::atomic<bool> active;

    ClientSession(crow::websocket::connection* conn, int min_val, int max_val)
        : connection(conn), value(min_val), value_min(min_val), value_max(max_val), active(true) {}
};

// Funktion zum Senden von Nachrichten an alle verbundenen Clients in der /ws Route
void message_sender(std::vector<crow::websocket::connection*>& connections, 
                    std::atomic<bool>& active, 
                    std::mutex& conn_mutex, 
                    int& value_min, 
                    int& value_max) {
    int value = value_min;

    while (active) {
        std::this_thread::sleep_for(3ms); 

        if (value >= value_max) {
            value = value_min;
        } else {
            value++;
        }

        // Zeitstempel mit Millisekunden erstellen
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

        // Nachricht als String konvertieren
        std::string message_str = message.dump();

        // Senden der Nachricht an alle verbundenen Clients in /ws
        std::lock_guard<std::mutex> lock(conn_mutex);
        for (auto conn_ptr : connections) {
            try {
                conn_ptr->send_text(message_str);
                std::cout << "Gesendete Nachricht: " << message_str << " an Client: " << conn_ptr << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "Fehler beim Senden an Client " << conn_ptr << ": " << e.what() << std::endl;
                active = false;
            }
        }
    }

    std::cout << "message_sender Thread beendet." << std::endl;
}

// Funktion zum Senden von Nachrichten an einen spezifischen Client in der /wsn Route
void individual_message_sender(ClientSession* session) {
    while (session->active) {
        std::this_thread::sleep_for(3ms);

        if (session->value >= session->value_max) {
            session->value = session->value_min;
        } else {
            session->value++;
        }

        // Zeitstempel mit Millisekunden erstellen
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;

        std::stringstream ss;
        ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%dT%H:%M:%S");
        ss << '.' << std::setfill('0') << std::setw(3) << milliseconds.count();

        // JSON-Nachricht erstellen
        json message = {
            {"timestamp", ss.str()},
            {"value", session->value.load()}
        };

        std::string message_str = message.dump();

        try {
            session->connection->send_text(message_str);
            std::cout << "Gesendete Nachricht: " << message_str << " an individuellen Client: " << session->connection << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Fehler beim Senden an individuellen Client " << session->connection << ": " << e.what() << std::endl;
            session->active = false;
        }
    }

    std::cout << "Individueller message_sender Thread für Client beendet: " << session->connection << std::endl;
}

int main() {
    crow::SimpleApp app;

    int value_min = 0;
    int value_max = 3;
    std::vector<crow::websocket::connection*> connections;
    std::mutex conn_mutex;
    auto active = std::make_shared<std::atomic<bool>>(true);

    std::unordered_map<crow::websocket::connection*, std::unique_ptr<ClientSession>> individual_sessions;
    std::mutex session_mutex;

    // Starten des message_sender Threads für die /ws Route
    std::thread([&connections, active, &conn_mutex, &value_min, &value_max]() {
        message_sender(connections, *active, conn_mutex, value_min, value_max);
    }).detach();

    // /ws Route für alle Clients mit gemeinsamer Konfiguration
    CROW_WEBSOCKET_ROUTE(app, "/ws")
    .onopen([&connections, &conn_mutex](crow::websocket::connection& conn) {
        std::cout << "WebSocket-Verbindung geöffnet für Client: " << &conn << std::endl;
        std::lock_guard<std::mutex> lock(conn_mutex);
        connections.push_back(&conn);
    })
    .onmessage([&value_min, &value_max](crow::websocket::connection& conn, const std::string& message, bool is_binary) {
        std::cout << "Nachricht vom Client empfangen: " << message << std::endl;
        json jsonData;
        try {
            jsonData = json::parse(message);
            if (jsonData.contains("Value_min") && jsonData.contains("Value_max")) {
                value_min = jsonData["Value_min"];
                value_max = jsonData["Value_max"];
                std::cout << "Empfangene Werte: Value_min = " << value_min << ", Value_max = " << value_max << std::endl;
            } else {
                std::cerr << "Ungültige Nachricht: Keine Value_min oder Value_max gefunden" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Fehler beim Parsen der Nachricht: " << e.what() << std::endl;
        }
    })
    .onclose([&connections, &conn_mutex](crow::websocket::connection& conn, const std::string& reason) {
        std::cout << "WebSocket-Verbindung geschlossen für Client: " << &conn << " Grund: " << reason << std::endl;
        std::lock_guard<std::mutex> lock(conn_mutex);
        connections.erase(std::remove(connections.begin(), connections.end(), &conn), connections.end());
    });

    // /wsi Route für individuelle Client-Sessions
    CROW_WEBSOCKET_ROUTE(app, "/wsi")
    .onopen([&individual_sessions, &session_mutex](crow::websocket::connection& conn) {
        std::cout << "Individuelle WebSocket-Verbindung geöffnet für Client: " << &conn << std::endl;
        int default_min = 0;
        int default_max = 3;
        {
            std::lock_guard<std::mutex> lock(session_mutex);
            individual_sessions[&conn] = std::make_unique<ClientSession>(&conn, default_min, default_max);
        }

        // Thread starten, um Nachrichten an den spezifischen Client zu senden
        std::thread([session = individual_sessions[&conn].get()]() {
            individual_message_sender(session);
        }).detach();
    })
    .onmessage([&individual_sessions, &session_mutex](crow::websocket::connection& conn, const std::string& message, bool is_binary) {
        std::cout << "Nachricht vom individuellen Client empfangen: " << message << std::endl;
        json jsonData;
        try {
            jsonData = json::parse(message);
            if (jsonData.contains("Value_min") && jsonData.contains("Value_max")) {
                int new_min = jsonData["Value_min"];
                int new_max = jsonData["Value_max"];

                std::lock_guard<std::mutex> lock(session_mutex);
                auto& session = individual_sessions[&conn];
                session->value_min = new_min;
                session->value_max = new_max;
                session->value = new_min;
                std::cout << "Empfangene Werte für individuellen Client: Value_min = " << new_min << ", Value_max = " << new_max << std::endl;
            } else {
                std::cerr << "Ungültige Nachricht: Keine Value_min oder Value_max gefunden" << std::endl;
            }
        } catch (const std::exception& e) {
            std::cerr << "Fehler beim Parsen der Nachricht: " << e.what() << std::endl;
        }
    })
    .onclose([&individual_sessions, &session_mutex](crow::websocket::connection& conn, const std::string& reason) {
        std::cout << "Individuelle WebSocket-Verbindung geschlossen für Client: " << &conn << " Grund: " << reason << std::endl;
        std::lock_guard<std::mutex> lock(session_mutex);
        individual_sessions[&conn]->active = false;
        individual_sessions.erase(&conn);
    });

    app.port(8765).multithreaded().run();
    return 0;
}
