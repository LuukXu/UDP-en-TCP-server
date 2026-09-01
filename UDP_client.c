#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
#define SERVER_IP "127.0.0.1"
#define PORT 12345
#define BUFFER_SIZE 1024

// Vraag 9: Struct om data netjes door te geven zonder global variables
typedef struct {
    SOCKET socket;
    int* is_waiting_for_result;
} ThreadArgs;

DWORD WINAPI ReceiveThread(LPVOID lpParam) {
    ThreadArgs* args = (ThreadArgs*)lpParam;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in server_addr;
    int server_len = sizeof(server_addr);

    while (1) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(args->socket, &readfds);

        struct timeval tv;
        tv.tv_sec = 12; // Time-out: we verwachten binnen 12s antwoord van de server
        tv.tv_usec = 0;

        int activity = select(0, &readfds, NULL, NULL, &tv);

        if (activity > 0) {
            int bytes = recvfrom(args->socket, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr*)&server_addr, &server_len);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                *(args->is_waiting_for_result) = 0; // We hebben antwoord, reset wachten
                printf("\n\n>>> Bericht van server: %s <<<\n", buffer);
                printf("Jouw gok: ");
                fflush(stdout);
            }
        } else if (activity == 0 && *(args->is_waiting_for_result) == 1) {
            // Vraag 3 & 5: Time-out & "You lost ?" printen
            printf("\n\n>>> Bericht van client: You lost ? (Time-out bereikt) <<<\n");
            *(args->is_waiting_for_result) = 0; // Stop met wachten op deze ronde
            printf("Jouw gok: ");
            fflush(stdout);
        }
    }
    return 0;
}

int main() {
    WSADATA wsa;
    struct sockaddr_in server_addr, client_bind_addr;
    char buffer[BUFFER_SIZE];
    
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;
    
    SOCKET client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    
    client_bind_addr.sin_family = AF_INET;
    client_bind_addr.sin_addr.s_addr = INADDR_ANY;
    client_bind_addr.sin_port = htons(0);
    bind(client_socket, (struct sockaddr*)&client_bind_addr, sizeof(client_bind_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    
    int is_waiting = 0;
    ThreadArgs args = { client_socket, &is_waiting };
    
    HANDLE thread = CreateThread(NULL, 0, ReceiveThread, (LPVOID)&args, 0, NULL);
    
    printf("--- UDP Client Gestart ---\n");
    
    while (1) {
        printf("Jouw gok: ");
        if (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
            buffer[strcspn(buffer, "\n")] = 0;
            if (strcmp(buffer, "exit") == 0) break;
            
            if (strlen(buffer) > 0) {
                sendto(client_socket, buffer, strlen(buffer), 0, (struct sockaddr*)&server_addr, sizeof(server_addr));
                is_waiting = 1; // Zet vlag om time-out monitoring te starten
            }
        }
    }
    
    closesocket(client_socket);
    WSACleanup();
    return 0;
}
