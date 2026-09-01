#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>
#include <windows.h> // Toegevoegd voor threads
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345

// --- DEZE FUNCTIE DRAAIT VOOR ELKE SPELER APART OP DE ACHTERGROND ---
DWORD WINAPI HandleSpeler(LPVOID lpParam) {
    // 1. Haal de specifieke client_socket op en maak het geheugen vrij
    SOCKET client_socket = *(SOCKET*)lpParam;
    free(lpParam);

    // 2. Iedere thread genereert een EIGEN geheim getal
    // We gebruiken de Thread ID om ervoor te zorgen dat ze echt andere getallen krijgen
    srand((unsigned int)(time(NULL) + GetCurrentThreadId()));
    int target_number = (rand() % 100) + 1;
    
    DWORD thread_id = GetCurrentThreadId();
    printf("[Thread %lu] Speler begonnen! Geheim getal is: %d\n", thread_id, target_number);

    // 3. De spel-lus voor DEZE specifieke speler
    while (1) {
        uint32_t net_guess;
        
        int bytes_received = recv(client_socket, (char*)&net_guess, sizeof(net_guess), 0);
        
        if (bytes_received <= 0) {
            printf("[Thread %lu] Speler heeft de verbinding verbroken.\n", thread_id);
            break; 
        }

        int guess = ntohl(net_guess);
        printf("[Thread %lu] Speler gokt: %d\n", thread_id, guess);

        if (guess < target_number) {
            const char* msg = "Hoger";
            send(client_socket, msg, strlen(msg), 0);
        } 
        else if (guess > target_number) {
            const char* msg = "Lager";
            send(client_socket, msg, strlen(msg), 0);
        } 
        else {
            const char* msg = "Correct";
            send(client_socket, msg, strlen(msg), 0);
            printf("[Thread %lu] Speler heeft gewonnen! Verbinding gesloten.\n", thread_id);
            break; 
        }
    }

    closesocket(client_socket);
    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    int client_len = sizeof(client_addr);

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) return 1;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) return 1;

    // SOMAXCONN laat Windows bepalen hoeveel mensen er maximaal in de wachtrij mogen staan
    listen(server_socket, SOMAXCONN); 
    printf("Multithreaded TCP-server 'Hoger/Lager' gestart op poort %d...\n", PORT);

    // De Hoofd-lus accepteert ALLEEN inkomende verbindingen
    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) continue;

        printf("\nNieuwe inkomende verbinding geaccepteerd (IP: %s)\n", inet_ntoa(client_addr.sin_addr));

        // We alloceren een klein stukje geheugen voor de socket. 
        // Dit is heel belangrijk bij multithreading zodat clients niet elkaars socket overschrijven!
        SOCKET* new_sock = (SOCKET*)malloc(sizeof(SOCKET));
        *new_sock = client_socket;

        // Start de thread voor deze specifieke client
        HANDLE thread = CreateThread(NULL, 0, HandleSpeler, (LPVOID)new_sock, 0, NULL);
        
        if (thread) {
            // Sluit de handle, de thread draait op de achtergrond gewoon door (detach)
            CloseHandle(thread); 
        } else {
            printf("Fout bij aanmaken thread.\n");
            free(new_sock);
            closesocket(client_socket);
        }
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
