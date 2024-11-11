# Build-Anleitung

## Voraussetzungen

### Für den C++ WebSocket-Server

Um den C++ WebSocket-Server zu bauen, benötigst du folgende Tools:

1. **CMake**: Ein Tool zur Verwaltung des Build-Prozesses.
   - Auf Ubuntu/Linux kannst du es mit folgendem Befehl installieren:

     ```bash
     sudo apt install cmake
     ```

2. **Crow WebSocket-Bibliothek**: 
   - Du musst die Crow-Bibliothek herunterladen und in deinem Projekt einbinden. Folge den Installationsanweisungen auf der [Crow GitHub-Seite](https://github.com/CrowCpp/Crow).

### Für den Python WebSocket-Client

1. **Python 3**:
   - Stelle sicher, dass Python 3 auf deinem System installiert ist. Du kannst die Version mit folgendem Befehl überprüfen:

     ```bash
     python3 --version
     ```

2. **Python-Abhängigkeiten installieren**:

   - Installiere die benötigten Python-Bibliotheken mit:

     ```bash
     pip install websocket-client matplotlib
     ```

## C++ Server kompilieren

1. **Erstelle das Build-Verzeichnis**:

   - Öffne ein Terminal im Projektverzeichnis und erstelle ein Verzeichnis für den Build-Prozess:

     ```bash
     mkdir build
     cd build
     ```

2. **Generiere Makefiles mit CMake**:

   - Im `build`-Verzeichnis führe den folgenden CMake-Befehl aus:

     ```bash
     cmake ..
     ```

3. **Baue das Projekt mit Make**:

   - Nachdem die Makefiles generiert wurden, baue das Projekt mit:

     ```bash
     make
     ```

4. **Starte den Server**:

   - Sobald der Build abgeschlossen ist, kannst du den Server mit folgendem Befehl starten:

     ```bash
     ./server
     ```

## Python-Client ausführen

1. **Stelle sicher, dass der Server läuft**.
2. **Starte den Client**:

   - Im Projektverzeichnis führe den Python-Client mit folgendem Befehl aus:

     ```bash
     python client.py
     ```

3. **Verwendung**:

   - Der Client zeigt Daten an, die vom Server gesendet werden. Du kannst den Client starten, stoppen und die letzten 100 Datenpunkte anzeigen.

## Fehlerbehebung

- Wenn der C++-Server nicht startet, stelle sicher, dass alle Abhängigkeiten korrekt installiert sind und dass keine anderen Programme Port 8765 blockieren.
- Wenn der Python-Client keine Verbindung herstellen kann, überprüfe die WebSocket-URI und stelle sicher, dass der Server läuft.

