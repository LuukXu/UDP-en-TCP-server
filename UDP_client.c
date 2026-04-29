
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h>   /* _beginthreadex */
 
#pragma comment(lib, "ws2_32.lib")
 
#define BUF_SIZE 256
 
static SOCKET g_sock   = INVALID_SOCKET;
static int    g_running = 1;
 
/* ── Luister-thread ── */
unsigned __stdcall listener(void *arg) {
    (void)arg;
    char buf[BUF_SIZE];
    struct sockaddr_in from;
    int from_len = sizeof(from);
 
    while (g_running) {
        int n = recvfrom(g_sock, buf, BUF_SIZE - 1, 0,
                         (struct sockaddr *)&from, &from_len);
        if (n == SOCKET_ERROR) {
            if (g_running)
                fprintf(stderr, "recvfrom fout: %d\n", WSAGetLastError());
            continue;
        }
        buf[n] = '\0';
 
        char *nl = strchr(buf, '\n'); if (nl) *nl = '\0';
        nl = strchr(buf, '\r');       if (nl) *nl = '\0';
 
        printf("\n  >>> Server zegt: '%s'\n", buf);
 
        if (strcmp(buf, "You won !") == 0 || strcmp(buf, "You lost !") == 0)
            printf("  Spel afgelopen.\n");
        else if (strcmp(buf, "You won ?") == 0)
            printf("  Je bent de winnaar! Druk Enter om te bevestigen...\n");
 
        fflush(stdout);
    }
    return 0;
}
 
int main(int argc, char *argv[]) {
    const char *server_ip   = (argc > 1) ? argv[1] : "127.0.0.1";
    int         server_port = (argc > 2) ? atoi(argv[2]) : 5005;
 
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup mislukt: %d\n", WSAGetLastError());
        return 1;
    }
 
    g_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (g_sock == INVALID_SOCKET) {
        fprintf(stderr, "socket() fout: %d\n", WSAGetLastError());
        WSACleanup(); return 1;
    }
 
    /* Bind lokale poort zodat de server kan antwoorden */
    struct sockaddr_in local;
    memset(&local, 0, sizeof(local));
    local.sin_family      = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port        = 0;
    if (bind(g_sock, (struct sockaddr *)&local, sizeof(local)) == SOCKET_ERROR) {
        fprintf(stderr, "bind() fout: %d\n", WSAGetLastError());
        closesocket(g_sock); WSACleanup(); return 1;
    }
 
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        fprintf(stderr, "Ongeldig server-adres: %s\n", server_ip);
        closesocket(g_sock); WSACleanup(); return 1;
    }
 
    /* Start luister-thread */
    HANDLE hThread = (HANDLE)_beginthreadex(NULL, 0, listener, NULL, 0, NULL);
    if (hThread == NULL) {
        fprintf(stderr, "Thread aanmaken mislukt\n");
        closesocket(g_sock); WSACleanup(); return 1;
    }
 
    printf("Verbonden met server %s:%d\n", server_ip, server_port);
    printf("Voer een geheel getal in als jouw gok (1-100).\n");
    printf("Druk Ctrl+C om te stoppen.\n\n");
 
    char buf[BUF_SIZE];
    while (1) {
        printf("Jouw gok: ");
        fflush(stdout);
 
        if (fgets(buf, sizeof(buf), stdin) == NULL) break;
 
        char *nl = strchr(buf, '\n'); if (nl) *nl = '\0';
        nl = strchr(buf, '\r');       if (nl) *nl = '\0';
 
        if (strlen(buf) == 0) continue;
 
        char *end;
        strtol(buf, &end, 10);
        if (end == buf || *end != '\0') {
            printf("  Voer een geldig geheel getal in.\n");
            continue;
        }
 
        int n = sendto(g_sock, buf, (int)strlen(buf), 0,
                       (const struct sockaddr *)&server_addr,
                       sizeof(server_addr));
        if (n == SOCKET_ERROR) {
            fprintf(stderr, "sendto fout: %d\n", WSAGetLastError());
            continue;
        }
        printf("  Verstuurd: %s\n", buf);
    }
 
    g_running = 0;
    closesocket(g_sock);
    WaitForSingleObject(hThread, 2000);
    CloseHandle(hThread);
    WSACleanup();
    return 0;
}
 