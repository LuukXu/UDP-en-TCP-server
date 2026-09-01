#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")
#define PORT 12345
#define MAX_CLIENTS 30

typedef struct {
    SOCKET sd;
    int target_number;
    struct sockaddr_in address;
} Client;

void log_action(const char* action, struct sockaddr_in* client) {
    FILE* log_file = fopen("tcp_server_log.txt", "a");
    if (log_file) {
        time_t now = time(NULL);
        char* t = ctime(&now);
        t[strlen(t)-1] = '\0';
        fprintf(log_file, "[%s] IP: %s:%d - %s\n", t, inet_ntoa(client->sin_addr), ntohs(client->sin_port), action);
        fclose(log_file);
    }
}

int main() {
    WSADATA wsa;
    SOCKET server_socket, new_socket;
    struct sockaddr_in server_addr, client_addr;
    Client clients[MAX_CLIENTS];
    int client_len = sizeof(client_addr);
    fd_set readfds;

    for (int i = 0; i < MAX_CLIENTS; i++) clients[i].sd = 0;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) return 1;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) return 1;
    listen(server_socket, 5);
    
    srand((unsigned int)time(NULL));
    printf("TCP Multiplexing Server gestart op poort %d...\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);
        SOCKET max_sd = server_socket;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].sd > 0) FD_SET(clients[i].sd, &readfds);
            if (clients[i].sd > max_sd) max_sd = clients[i].sd;
        }

        int activity = select(0, &readfds, NULL, NULL, NULL);
        if (activity < 0) continue;

        // Inkomende connectie afhandelen
        if (FD_ISSET(server_socket, &readfds)) {
            new_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
            
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].sd == 0) {
                    clients[i].sd = new_socket;
                    clients[i].address = client_addr;
                    clients[i].target_number = rand() % 1000001; 
                    
                    char log_msg[256];
                    sprintf(log_msg, "Nieuwe speler verbonden. Doelgetal: %d", clients[i].target_number);
                    log_action(log_msg, &client_addr);
                    
                    // ---> HIER IS DE PRINTF TOEGEVOEGD <---
                    printf("\nNieuwe speler verbonden (IP: %s) | Doelgetal: %d\n", inet_ntoa(client_addr.sin_addr), clients[i].target_number);
                    break;
                }
            }
        }

        // Gokken van actieve clients afhandelen
        for (int i = 0; i < MAX_CLIENTS; i++) {
            SOCKET sd = clients[i].sd;
            if (sd > 0 && FD_ISSET(sd, &readfds)) {
                uint32_t net_guess;
                int bytes = recv(sd, (char*)&net_guess, sizeof(net_guess), 0);

                if (bytes <= 0) {
                    // ---> HIER IS EEN PRINTF TOEGEVOEGD <---
                    printf("Speler (IP: %s) heeft de verbinding verbroken.\n", inet_ntoa(clients[i].address.sin_addr));
                    closesocket(sd);
                    clients[i].sd = 0;
                } else {
                    int guess = ntohl(net_guess); 
                    
                    // ---> HIER IS EEN PRINTF TOEGEVOEGD <---
                    printf("Speler (IP: %s) gokt: %d\n", inet_ntoa(clients[i].address.sin_addr), guess);
                    
                    if (guess < clients[i].target_number) {
                        send(sd, "Hoger", 5, 0);
                    } else if (guess > clients[i].target_number) {
                        send(sd, "Lager", 5, 0);
                    } else {
                        send(sd, "Correct", 7, 0);
                        log_action("Speler heeft gewonnen en is losgekoppeld.", &clients[i].address);
                        
                        // ---> HIER IS EEN PRINTF TOEGEVOEGD <---
                        printf("Speler (IP: %s) heeft gewonnen! Verbinding wordt gesloten.\n", inet_ntoa(clients[i].address.sin_addr));
                        
                        closesocket(sd);
                        clients[i].sd = 0; 
                    }
                }
            }
        }
    }
    closesocket(server_socket);
    WSACleanup();
    return 0;
}
