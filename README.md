# UDP-en-TCP-server
# Netwerken Opdrachten: UDP & TCP Socket Programmeren

Dit project bevat twee netwerkapplicaties geschreven in **C** met behulp van de **Winsock2 API** voor Windows. De opdrachten demonstreren de verschillen tussen verbindingloos netwerkverkeer (UDP) en verbindingsgericht netwerkverkeer (TCP), inclusief het gebruik van multithreading en netwerk-byte-order.

---

## 🛠️ Vereisten
Om deze code te compileren en uit te voeren heb je het volgende nodig:
* Een Windows-omgeving (vereist voor `winsock2.h` en `windows.h`).
* Een C-compiler zoals **MinGW (GCC)** geïnstalleerd en geconfigureerd.
* Een terminal (zoals PowerShell of VS Code terminal).

---

## 1. UDP-Server & Client: "Wie is het dichtste?"

Een tijdsgebonden raadspel over UDP waarbij spelers zo dicht mogelijk bij een geheim getal (1-100) moeten gokken. Snelheid en discipline zijn cruciaal.

### Spelregels & Werking
* **De Start:** De server ontwaakt zodra de allereerste speler een getal (in ASCII) verstuurt. Er wordt een geheim doelgetal gegenereerd en een timer van 8 seconden gestart.
* **Halverende Timer:** Bij elke nieuwe gok van een client wordt gecontroleerd of deze dichterbij is. Is dat het geval, dan wordt deze speler de nieuwe koploper. **Let op:** bij elke ontvangen gok wordt de resterende timer *gehalveerd en gereset*.
* **De 16-seconden Valstrik:** Als de timer afloopt, krijgt de koploper het bericht `"You won ?"`. Vanaf dat moment is er 16 seconden radiostilte. Als *iemand* (inclusief de winnaar) in deze tijd een bericht stuurt, volgt de straf: `"You lost !"`.
* **Overwinning:** Blijft de winnaar 16 seconden stil, dan ontvangt deze definitief `"You won !"`.
* **Multithreaded Client:** De client gebruikt een aparte achtergrondthread om netwerkberichten te ontvangen zonder de toetsenbordinvoer (`fgets`) te blokkeren.

### Compileren en Starten
Zorg dat je in de map van de UDP-opdracht zit en voer uit:

```bash
# Compileer de server en client
gcc server.c -o server.exe -lws2_32
gcc client.c -o client.exe -lws2_32

# Start in Terminal 1:
./server.exe

# Start in Terminal 2 (en 3...):
./client.exe
