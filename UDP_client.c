#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h> // Voor CreateThread en Sleep

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 12345
#define BUFFER_SIZE 1024

SOCKET client_socket;

// De achtergrondtaak die constant blijft luisteren
DWORD WINAPI ReceiveThread(LPVOID lpParam) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    int server_len = sizeof(server_addr);

    while (1) {
        int bytes = recvfrom(client_socket, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&server_addr, &server_len);
        
        if (bytes > 0) {
            buffer[bytes] = '\0';
            printf("\n\n>>> Bericht van server: %s <<<\n", buffer);
            printf("Jouw gok: "); 
            fflush(stdout); // Zorg dat de tekst direct op het scherm forceert
        } else {
            // FIX: Sluit de thread niet meer af bij een error (break), 
            // maar wacht een fractie van een seconde en luister gewoon verder!
            Sleep(50);
        }
    }
    return 0;
}

int main() {
    WSADATA wsa;
    struct sockaddr_in server_addr, client_bind_addr;
    char buffer[BUFFER_SIZE];
    
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
    if ((client_socket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) return 1;
    
    // FIX: Geef de client een expliciete lokale poort voordat de thread start.
    // Door poort '0' in te vullen, kiest Windows zelf een vrije poort.
    client_bind_addr.sin_family = AF_INET;
    client_bind_addr.sin_addr.s_addr = INADDR_ANY;
    client_bind_addr.sin_port = htons(0); 
    bind(client_socket, (struct sockaddr*)&client_bind_addr, sizeof(client_bind_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    printf("--- UDP Client Gestart ---\n");
    printf("Typ een getal en druk op Enter om te gokken.\n");
    printf("Typ 'exit' om te stoppen.\n\n");
    
    // Start de ontvangst-thread
    HANDLE thread = CreateThread(NULL, 0, ReceiveThread, NULL, 0, NULL);

    // Hoofdlus: Wachten op toetsenbordinvoer
    while (1) {
        printf("Jouw gok: ");
        if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
            
            buffer[strcspn(buffer, "\n")] = 0;
            if (strcmp(buffer, "exit") == 0) break;
            
            // Stuur het getal naar de server
            if (strlen(buffer) > 0) {
                sendto(client_socket, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
            }
        }
    }
    
    closesocket(client_socket);
    WSACleanup();
    return 0;
}
