#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 12345
#define BUFFER_SIZE 1024

// Hulpfunctie om te controleren of twee clients exact hetzelfde zijn (IP + Poort)
int is_same_client(struct sockaddr_in* a, struct sockaddr_in* b) {
    return (a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port);
}

int main() {
    WSADATA wsa;
    SOCKET server_socket;
    struct sockaddr_in server_addr;

    // 1. Initialiseer Winsock
    printf("Initialiseren van Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Fout bij WSAStartup. Error Code : %d\n", WSAGetLastError());
        return 1;
    }

    // 2. Maak de UDP socket aan
    if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) {
        printf("Kan socket niet aanmaken : %d\n", WSAGetLastError());
        return 1;
    }

    // 3. Bind de socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind mislukt met error code : %d\n", WSAGetLastError());
        return 1;
    }

    printf("UDP-server succesvol gestart op poort %d.\n", PORT);
    srand((unsigned int)time(NULL));

    // Hoofd-lus van de server (per ronde)
    while (1) {
        char buffer[BUFFER_SIZE];
        struct sockaddr_in client_addr, best_client;
        int client_len = sizeof(client_addr);
        
        printf("\n=== Wachten op een eerste gok om de ronde te starten ===\n");

        // FASE 1: Wacht oneindig op het allereerste bericht
        int bytes = recvfrom(server_socket, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&client_addr, &client_len);
        if (bytes == SOCKET_ERROR) continue;
        buffer[bytes] = '\0';
        
        int first_guess = atoi(buffer);
        int target_number = (rand() % 100) + 1; // Genereer doelgetal (1-100)
        
        int best_guess = first_guess;
        int best_diff = abs(target_number - first_guess);
        best_client = client_addr;
        
        double current_timeout = 8.0;

        printf("Nieuwe ronde! Doelgetal is: %d\n", target_number);
        printf("Eerste gok ontvangen: %d | Time-out gezet op: %.2f seconden\n", first_guess, current_timeout);

        // FASE 2: Gokfase met halverende time-out
        while (1) {
            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(server_socket, &readfds);

            struct timeval tv;
            tv.tv_sec = (long)current_timeout;
            tv.tv_usec = (long)((current_timeout - tv.tv_sec) * 1000000.0);

            // select() luistert naar de socket tot de time-out verloopt
            int activity = select(0, &readfds, NULL, NULL, &tv);

            if (activity == 0) {
                // Time-out bereikt! (Geen nieuwe berichten binnen de tijd)
                printf("Time-out event! Einde van de gokfase.\n");
                break;
            } else if (activity > 0) {
                // Nieuw bericht ontvangen binnen de tijd
                bytes = recvfrom(server_socket, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&client_addr, &client_len);
                if (bytes > 0) {
                    buffer[bytes] = '\0';
                    int guess = atoi(buffer);
                    int diff = abs(target_number - guess);

                    if (diff < best_diff) {
                        best_diff = diff;
                        best_guess = guess;
                        best_client = client_addr;
                    }

                    // Halveer de time-out voor de volgende iteratie
                    current_timeout /= 2.0;
                    printf("Nieuwe gok: %d | Resterende time-out gehalveerd naar: %.2f sec\n", guess, current_timeout);
                }
            }
        }

        // FASE 3: Informeer de voorlopige winnaar
        const char* msg_won_question = "You won ?";
        sendto(server_socket, msg_won_question, strlen(msg_won_question), 0, (struct sockaddr*)&best_client, sizeof(best_client));
        printf("Voorlopige winnaar gecontacteerd: '%s' (Gok: %d)\n", msg_won_question, best_guess);

        // FASE 4: De 16-seconden controlefase
        ULONGLONG end_time = GetTickCount64() + 16000; // Huidige tijd + 16000 milliseconden
        int received_from_winner = 0;

        printf("Start 16-seconden controlefase...\n");

        while (1) {
            ULONGLONG now = GetTickCount64();
            if (now >= end_time) {
                break; // 16 seconden zijn voorbij
            }

            ULONGLONG remaining_ms = end_time - now;
            struct timeval tv_16s;
            tv_16s.tv_sec = (long)(remaining_ms / 1000);
            tv_16s.tv_usec = (long)((remaining_ms % 1000) * 1000);

            fd_set readfds;
            FD_ZERO(&readfds);
            FD_SET(server_socket, &readfds);

            int activity = select(0, &readfds, NULL, NULL, &tv_16s);

            if (activity == 0) {
                // Volledige 16s verstreken zonder extra activiteit
                break;
            } else if (activity > 0) {
                // Er kwam nog een (laat) bericht binnen
                bytes = recvfrom(server_socket, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&client_addr, &client_len);
                if (bytes > 0) {
                    const char* msg_lost = "You lost !";
                    sendto(server_socket, msg_lost, strlen(msg_lost), 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
                    
                    // Controleer of het de winnaar was die de fout maakte nog iets te sturen
                    if (is_same_client(&client_addr, &best_client)) {
                        received_from_winner = 1;
                        printf("Winnaar stuurde nog een bericht en verliest alsnog!\n");
                    } else {
                        printf("Laat bericht van verliezer afgehandeld met '%s'\n", msg_lost);
                    }
                }
            }
        }

        // FASE 5: Definitieve bevestiging
        if (!received_from_winner) {
            const char* msg_won_exclaim = "You won !";
            sendto(server_socket, msg_won_exclaim, strlen(msg_won_exclaim), 0, (struct sockaddr*)&best_client, sizeof(best_client));
            printf("Ronde afgelopen! Definitieve '%s' verstuurd naar de winnaar.\n", msg_won_exclaim);
        }
    }

    // Sluit netjes af (wordt in deze oneindige loop normaal niet bereikt)
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
