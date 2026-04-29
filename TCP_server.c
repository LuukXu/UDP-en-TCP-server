#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <stdint.h>
#include <process.h>
 
#pragma comment(lib, "ws2_32.lib")
 
#define PORT    5006
#define BACKLOG 10
 
typedef struct {
    SOCKET             sock;
    struct sockaddr_in addr;
} ClientInfo;
 
unsigned __stdcall client_thread(void *arg) {
    ClientInfo *info   = (ClientInfo *)arg;
    SOCKET client_sock = info->sock;
    char  *client_ip   = inet_ntoa(info->addr.sin_addr);
    int    client_port = ntohs(info->addr.sin_port);
    free(info);
    srand((unsigned)time(NULL) ^ (unsigned)GetCurrentThreadId());
    int secret = rand() % 1000000 + 1;
    printf("\n[+] Client verbonden: %s:%d  |  Geheim getal: %d\n",
           client_ip, client_port, secret);
 
    int spel_actief = 1;
    while (spel_actief) {
        uint32_t net_val         = 0;
        int      bytes_ontvangen = 0;
        char    *buf             = (char *)&net_val;
 
        while (bytes_ontvangen < 4) {
            int n = recv(client_sock, buf + bytes_ontvangen,
                         4 - bytes_ontvangen, 0);
            if (n <= 0) {
                printf("    [%s:%d] Verbinding verbroken.\n",
                       client_ip, client_port);
                spel_actief = 0;
                break;
            }
            bytes_ontvangen += n;
        }
 
        if (!spel_actief) break;
 
        int guess = (int)ntohl(net_val);
        printf("    [%s:%d] Gok: %d\n", client_ip, client_port, guess);
 
        const char *antwoord;
        if (guess < secret)
            antwoord = "Hoger\n";
        else if (guess > secret)
            antwoord = "Lager\n";
        else
            antwoord = "Correct\n";
 
        send(client_sock, antwoord, (int)strlen(antwoord), 0);
 
        if (guess == secret) {
            printf("    [%s:%d] Correct geraden!\n", client_ip, client_port);
            spel_actief = 0;
        }
    }
 
    closesocket(client_sock);
    printf("[-] Client losgekoppeld: %s:%d\n", client_ip, client_port);
    return 0;
}
 
int main(void) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup mislukt: %d\n", WSAGetLastError());
        return 1;
    }
 
    srand((unsigned)time(NULL));
 
    SOCKET server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_sock == INVALID_SOCKET) {
        fprintf(stderr, "socket() fout: %d\n", WSAGetLastError());
        WSACleanup(); return 1;
    }
 
    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));
 
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);
 
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind() fout: %d\n", WSAGetLastError());
        closesocket(server_sock); WSACleanup(); return 1;
    }
 
    if (listen(server_sock, BACKLOG) == SOCKET_ERROR) {
        fprintf(stderr, "listen() fout: %d\n", WSAGetLastError());
        closesocket(server_sock); WSACleanup(); return 1;
    }
 
    printf("TCP-server 'Hoger/Lager' gestart op poort %d\n", PORT);
    printf("Wacht op verbindingen (meerdere clients tegelijk)...\n");
 
    for (;;) {
        struct sockaddr_in client_addr;
        int addr_len = sizeof(client_addr);
 
        SOCKET client_sock = accept(server_sock,
                                    (struct sockaddr *)&client_addr,
                                    &addr_len);
        if (client_sock == INVALID_SOCKET) {
            fprintf(stderr, "accept() fout: %d\n", WSAGetLastError());
            continue;
        }
 
        ClientInfo *info = malloc(sizeof(ClientInfo));
        info->sock = client_sock;
        info->addr = client_addr;
 
        HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, client_thread, info, 0, NULL);
        if (hThread == NULL) {
            fprintf(stderr, "Thread aanmaken mislukt\n");
            closesocket(client_sock);
            free(info);
        } else {
            CloseHandle(hThread);
        }
    }
 
    closesocket(server_sock);
    WSACleanup();
    return 0;
}