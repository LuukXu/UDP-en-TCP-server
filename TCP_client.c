#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")
#define SERVER_IP "127.0.0.1"
#define PORT 12345

void play_game(struct sockaddr_in* server_addr) {
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == INVALID_SOCKET) return;

    if (connect(client_socket, (struct sockaddr*)server_addr, sizeof(*server_addr)) < 0) {
        printf("Kan niet verbinden met server.\n");
        return;
    }
    
    printf("\n--- Nieuw Spel Gestart! Gok tussen 0 en 1.000.000 ---\n");
    char input_buffer[256], response_buffer[256];

    while (1) {
        printf("Jouw gok: ");
        if (fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
            uint32_t guess = (uint32_t)atoi(input_buffer);
            uint32_t net_guess = htonl(guess); // Vraag 12: NBO

            if (send(client_socket, (char*)&net_guess, sizeof(net_guess), 0) == SOCKET_ERROR) break;

            int bytes = recv(client_socket, response_buffer, sizeof(response_buffer) - 1, 0);
            if (bytes > 0) {
                response_buffer[bytes] = '\0';
                printf("Server: %s\n\n", response_buffer);

                if (strcmp(response_buffer, "Correct") == 0) {
                    printf("Gefeliciteerd! Je hebt het juist.\n");
                    break;
                }
            } else {
                printf("Verbinding verbroken door de server.\n");
                break;
            }
        }
    }
    closesocket(client_socket); // Vraag 14: Proper afgesloten
}

int main() {
    WSADATA wsa;
    struct sockaddr_in server_addr;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    char choice[10];
    
    // Vraag 15: Continuous gaming
    do {
        play_game(&server_addr);
        printf("\nWil je nog een keer spelen? (j/n): ");
        fgets(choice, sizeof(choice), stdin);
    } while (choice[0] == 'j' || choice[0] == 'J');

    WSACleanup();
    return 0;
}
