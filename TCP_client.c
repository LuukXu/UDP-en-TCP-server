#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdint.h>
 
#pragma comment(lib, "ws2_32.lib")
 
#define BUF_SIZE 64
 
int main(int argc, char *argv[]) {
    const char *server_ip   = (argc > 1) ? argv[1] : "127.0.0.1";
    int         server_port = (argc > 2) ? atoi(argv[2]) : 5006;
 
    /* ── Winsock initialiseren ── */
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup mislukt: %d\n", WSAGetLastError());
        return 1;
    }
 
    /* ── Socket aanmaken ── */
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "socket() fout: %d\n", WSAGetLastError());
        WSACleanup(); return 1;
    }
 
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(server_port);

    // Gebruik inet_pton in plaats van inet_addr
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Ongeldig server-adres: %s\n", server_ip);
        closesocket(sock); 
        WSACleanup(); 
        return 1;
    }
 
    /* ── Verbinden met server ── */
    if (connect(sock, (struct sockaddr *)&server_addr,
                sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "connect() fout: %d\n", WSAGetLastError());
        closesocket(sock); WSACleanup(); return 1;
    }
 
    printf("Verbonden met server %s:%d\n", server_ip, server_port);
    printf("Raad het getal (1-1000000)!\n\n");
 
    char input[BUF_SIZE];
    char response[BUF_SIZE];
 
    while (1) {
        /* ── Invoer van gebruiker ── */
        printf("Jouw gok: ");
        fflush(stdout);
 
        if (fgets(input, sizeof(input), stdin) == NULL) break;
 
        /* Verwijder newline */
        char *nl = strchr(input, '\n'); if (nl) *nl = '\0';
        nl = strchr(input, '\r');       if (nl) *nl = '\0';
 
        if (strlen(input) == 0) continue;
 
        /* Valideer: is het een getal? */
        char *end;
        long guess = strtol(input, &end, 10);
        if (end == input || *end != '\0') {
            printf("  Voer een geldig geheel getal in.\n");
            continue;
        }
 
        /* ── Stuur als 32-bit integer in network-byte-order ── */
        uint32_t net_val = htonl((uint32_t)(int)guess);
        int sent = send(sock, (const char *)&net_val, 4, 0);
        if (sent == SOCKET_ERROR) {
            fprintf(stderr, "send() fout: %d\n", WSAGetLastError());
            break;
        }
 
        /* ── Ontvang antwoord van server ── */
        memset(response, 0, sizeof(response));
        int n = recv(sock, response, BUF_SIZE - 1, 0);
        if (n <= 0) {
            printf("  Server heeft verbinding verbroken.\n");
            break;
        }
        response[n] = '\0';
 
        /* Verwijder newline voor nette weergave */
        nl = strchr(response, '\n'); if (nl) *nl = '\0';
        nl = strchr(response, '\r'); if (nl) *nl = '\0';
 
        printf("  Server: %s\n\n", response);
 
        /* Spel voorbij bij "Correct" */
        if (strcmp(response, "Correct") == 0) {
            printf("Gefeliciteerd! Je hebt het getal geraden!\n");
            break;
        }
    }
 
    closesocket(sock);
    WSACleanup();
    printf("Verbinding gesloten.\n");
    return 0;
}