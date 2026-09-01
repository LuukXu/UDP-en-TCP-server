#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")
#define PORT 12345
#define BUFFER_SIZE 1024

// Extra functie: Logging naar een bestand (Vraag 10)
void log_action(const char* action, struct sockaddr_in* client) {
    FILE* log_file = fopen("udp_server_log.txt", "a");
    if (log_file) {
        time_t now = time(NULL);
        char* t = ctime(&now);
        t[strlen(t)-1] = '\0'; // Verwijder newline
        fprintf(log_file, "[%s] IP: %s:%d - %s\n", t, inet_ntoa(client->sin_addr), ntohs(client->sin_port), action);
        fclose(log_file);
    }
}

int is_same_client(struct sockaddr_in* a, struct sockaddr_in* b) {
    return (a->sin_addr.s_addr == b->sin_addr.s_addr && a->sin_port == b->sin_port);
}

void run_game(SOCKET server_socket) {
    char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr, best_client;
    int client_len = sizeof(client_addr);
    
    printf("\n=== Wachten op een eerste gok om de ronde te starten ===\n");

    int bytes = recvfrom(server_socket, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&client_addr, &client_len);
    if (bytes == SOCKET_ERROR) return;
    
    buffer[bytes] = '\0';
    int first_guess = atoi(buffer);
    
    // Vraag 2: Getal tussen 0 en 99
    int target_number = rand() % 100; 
    
    int best_guess = first_guess;
    int best_diff = abs(target_number - first_guess);
    best_client = client_addr;
    
    double current_timeout = 8.0;
    
    char log_msg[256];
    sprintf(log_msg, "Nieuwe ronde gestart. Doelgetal: %d. Eerste gok: %d", target_number, first_guess);
    log_action(log_msg, &client_addr);

    printf("Nieuwe ronde! Doelgetal: %d\n", target_number);

    // Gokfase
    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);

        struct timeval tv;
        tv.tv_sec = (long)current_timeout;
        tv.tv_usec = (long)((current_timeout - tv.tv_sec) * 1000000.0);

        int activity = select(0, &readfds, NULL, NULL, &tv);

        if (activity == 0) {
            break; // Time-out event (Vraag 3)
        } else if (activity > 0) {
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
                current_timeout /= 2.0; // Vraag 4: Dynamic Timeout
            }
        }
    }

    // Voorlopige winnaar
    sendto(server_socket, "You won ?", 9, 0, (struct sockaddr*)&best_client, sizeof(best_client));
    log_action("Voorlopige winnaar (You won ?)", &best_client);

    // 16 seconden controlefase (Vraag 6)
    ULONGLONG end_time = GetTickCount64() + 16000; 
    int received_from_winner = 0;

    while (1) {
        ULONGLONG now = GetTickCount64();
        if (now >= end_time) break;

        struct timeval tv_16s;
        tv_16s.tv_sec = (long)((end_time - now) / 1000);
        tv_16s.tv_usec = (long)(((end_time - now) % 1000) * 1000);

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);

        int activity = select(0, &readfds, NULL, NULL, &tv_16s);

        if (activity > 0) {
            bytes = recvfrom(server_socket, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&client_addr, &client_len);
            if (bytes > 0) {
                sendto(server_socket, "You lost !", 10, 0, (struct sockaddr*)&client_addr, sizeof(client_addr));
                log_action("Laat bericht afgehandeld (You lost !)", &client_addr);
                if (is_same_client(&client_addr, &best_client)) received_from_winner = 1;
            }
        }
    }

    // Vraag 7: Definitieve winnaar
    if (!received_from_winner) {
        sendto(server_socket, "You won !", 9, 0, (struct sockaddr*)&best_client, sizeof(best_client));
        log_action("Definitieve overwinning (You won !)", &best_client);
    }
}

int main() {
    WSADATA wsa;
    SOCKET server_socket;
    struct sockaddr_in server_addr;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
    if ((server_socket = socket(AF_INET, SOCK_DGRAM, 0)) == INVALID_SOCKET) return 1;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) return 1;

    srand((unsigned int)time(NULL));
    
    // Vraag 8: Continuous
    while (1) {
        run_game(server_socket);
    }

    closesocket(server_socket);
    WSACleanup();
    return 0;
}
