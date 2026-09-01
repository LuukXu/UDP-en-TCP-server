#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 12345

int main() {
    WSADATA wsa;
    SOCKET client_socket;
    struct sockaddr_in server_addr;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) return 1;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);

    // Verbind met de TCP-server (Connect)
    printf("Verbinden met TCP Server...\n");
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("Kan niet verbinden met de server.\n");
        return 1;
    }
    printf("Verbonden! Start met gokken.\n\n");

    char input_buffer[256];
    char response_buffer[256];

    while (1) {
        printf("Jouw gok: ");
        if (fgets(input_buffer, sizeof(input_buffer), stdin) != NULL) {
            
            // Converteer de getypte tekst naar een integer
            uint32_t guess = (uint32_t)atoi(input_buffer);
            
            // Converteer de integer van Host-Byte-Order naar Network-Byte-Order met htonl()
            uint32_t net_guess = htonl(guess);

            // Verstuur de rauwe 4 bytes
            if (send(client_socket, (char*)&net_guess, sizeof(net_guess), 0) == SOCKET_ERROR) {
                printf("Fout bij verzenden.\n");
                break;
            }

            // Wacht op de string (Hoger, Lager, of Correct)
            int bytes = recv(client_socket, response_buffer, sizeof(response_buffer) - 1, 0);
            if (bytes > 0) {
                response_buffer[bytes] = '\0'; // Zorg dat de string netjes stopt
                printf("Server: %s\n\n", response_buffer);

                if (strcmp(response_buffer, "Correct") == 0) {
                    printf("Je hebt gewonnen! Verbinding wordt gesloten.\n");
                    break;
                }
            } else {
                printf("Server heeft de verbinding verbroken.\n");
                break;
            }
        }
    }

    // Disconnect
    closesocket(client_socket);
    WSACleanup();
    return 0;
}
