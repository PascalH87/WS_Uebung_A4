# WS_Uebung_A4
Multi Producer - Single Consumer

# Distributed WebSocket System mit Sägezahn- und Sinus-Servern

Dieses Projekt besteht aus zwei WebSocket-Servern, die auf verschiedenen Ports laufen und unterschiedliche Wellenformen erzeugen: ein **Sägezahn-Wellen-Server** und ein **Sinus-Wellen-Server**. Beide Server senden ihre jeweiligen Daten an einen Python-Client, der diese empfängt und in Echtzeit visualisiert.

## Projektstruktur

- `client.py`: Python WebSocket-Client, der Daten von beiden WebSocket-Servern empfängt und visualisiert.
- `sine_wave_server.cpp`: C++ WebSocket-Server, der sinusförmige Daten mit einer Zeitkonstanten von 7ms sendet.
- `sawtooth_wave_server.cpp`: C++ WebSocket-Server, der sägezahnförmige Daten mit einer Zeitkonstanten von 3ms sendet.
- `Crow`: Eine C++ WebSocket-Bibliothek, die im Serverprojekt verwendet wird.
- `CMakeLists.txt`: Build-Datei für das C++-Projekt.
- `README.md`: Diese Datei, die Informationen zum Setup und der Nutzung des Projekts enthält.

## Voraussetzungen

### Für den C++ WebSocket-Server:

1. **Crow**-Bibliothek:
   - Du musst die Crow-Bibliothek installieren, um den Server zu bauen.
   - Weitere Informationen findest du hier: [Crow GitHub Repository](https://github.com/CrowCpp/Crow)

2. **CMake**:
   - Zum Erstellen des C++-Projekts benötigst du CMake. Installiere es mit:

     `sudo apt install cmake`

### Für den Python WebSocket-Client:

1. **Python 3**:
   - Stelle sicher, dass Python 3 auf deinem System installiert ist.

2. **Python Abhängigkeiten**:
   - Du solltest eine virtuelle Python-Umgebung (venv) erstellen, um die Abhängigkeiten zu installieren:

     ```bash
     python3 -m venv ~/venv
     source ~/venv/bin/activate  # Aktiviert die virtuelle Umgebung
     pip install websocket-client matplotlib numpy
     ```

## Kompilieren und Ausführen

### C++ WebSocket-Server (sine_wave_server.cpp und sawtooth_wave_server.cpp)

1. **CMake-Build-Prozess**:

   - Erstelle ein Verzeichnis `build` im Projektordner:

     ```bash
     mkdir build
     cd build
     ```

   - Generiere die Makefiles und baue das Projekt:

     ```bash
     cmake ..
     make
     ```

   - Der Server wird jetzt erstellt und kann mit folgendem Befehl gestartet werden:

     ```bash
     ./sine_wave_server  # Für den Sinus-Wellen-Server auf Port 8766
     ./sawtooth_wave_server  # Für den Sägezahn-Wellen-Server auf Port 8767
     ```

2. Die C++-Server laufen nun auf den Ports **8766** (Sinus-Welle) und **8767** (Sägezahn-Welle) und senden Daten an verbundene Clients.

### Python WebSocket-Client (client.py)

1. **Starten des Python-Clients**:

   - Stelle sicher, dass beide C++-Server laufen.
   - Aktiviere die Python-virtuelle Umgebung:

     ```bash
     source ~/venv/bin/activate
     ```

   - Führe das Python-Skript aus, um den Client zu starten:

     ```bash
     python client.py
     ```

   - Du kannst die WebSocket-URIs in der `client.py`-Datei anpassen, falls die Server auf anderen Hosts oder Ports laufen.

### Funktionen des Clients:

- **Datenempfang**: Der Client empfängt Daten sowohl vom Sinus-Wellen-Server als auch vom Sägezahn-Wellen-Server.
- **Visualisierung**: Der Client zeigt die Wellenformen in Echtzeit an, wobei die Sinus-Wellen-Daten alle 7ms und die Sägezahn-Wellen-Daten alle 3ms empfangen werden.
- **Oszilloskop-Plot**: Alle **20ms** wird das Oszilloskop neu gezeichnet, basierend auf den neuesten Daten aus beiden Quellen.
- **Datenstruktur**: Der Client speichert die Daten der beiden Wellenformen in separaten Datenstrukturen und nutzt diese, um das Oszilloskop-Plot zu aktualisieren.

### Python GUI Workflow:

1. **Verbindung zu den WebSocket-Servern**:
   - Der Client verbindet sich mit dem Sinus-Wellen-Server auf Port **8766** und dem Sägezahn-Wellen-Server auf Port **8767**.

2. **Datenempfang und -verarbeitung**:
   - Die empfangenen Daten werden in separaten Listen gespeichert: eine für die Sinus-Welle und eine für die Sägezahn-Welle.

3. **Visualisierung**:
   - Alle **20ms** wird das Oszilloskop-Plot basierend auf den letzten 100 Datenpunkten für beide Wellenformen aktualisiert.

## Lizenz

Dieses Projekt ist unter der MIT-Lizenz lizenziert - siehe [LICENSE](LICENSE) für Details.

