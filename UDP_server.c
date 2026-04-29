#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
 
#pragma comment(lib, "ws2_32.lib")
 
#define PORT            5005
#define BUF_SIZE        256
#define INITIAL_TIMEOUT 8.0
#define CONFIRM_TIMEOUT 16.0
#define MAX_PLAYERS     64
 
double now_sec(void) {
    LARGE_INTEGER freq, cnt;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&cnt);
    return (double)cnt.QuadPart / (double)freq.QuadPart;
}
 
int addr_eq(const struct sockaddr_in *a, const struct sockaddr_in *b) {
    return a->sin_addr.s_addr == b->sin_addr.s_addr &&
           a->sin_port         == b->sin_port;
}
 
void send_msg(SOCKET sock, const char *msg, const struct sockaddr_in *addr) {
    sendto(sock, msg, (int)strlen(msg), 0,
           (const struct sockaddr *)addr, sizeof(*addr));
}
 
typedef struct {
    struct sockaddr_in addr;
    int                last_guess;
} Player;
 
int main(void) {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "WSAStartup mislukt: %d\n", WSAGetLastError());
        return 1;
    }
 
    srand((unsigned)time(NULL));
 
    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "socket() fout: %d\n", WSAGetLastError());
        WSACleanup(); return 1;
    }
 
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);
 
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        fprintf(stderr, "bind() fout: %d\n", WSAGetLastError());
        closesocket(sock); WSACleanup(); return 1;
    }
 
    printf("UDP-server gestart op poort %d\n", PORT);
 
    for (;;) {
        int secret = rand() % 100 + 1;
        printf("\n==================================================\n");
        printf("  Nieuw spel! Geheim getal: %d  (1-100)\n", secret);
        printf("==================================================\n");
 
        struct sockaddr_in closest_addr;
        memset(&closest_addr, 0, sizeof(closest_addr));
        int closest_value = -1;
        int closest_diff  = 999999;
        int has_winner    = 0;
 
        Player players[MAX_PLAYERS];
        int    player_count = 0;
 
        double timeout_at = 0.0;
        int    phase      = 0;
 
        while (1) {
            double remaining;
            if (timeout_at == 0.0)
                remaining = 60.0;
            else {
                remaining = timeout_at - now_sec();
                if (remaining < 0.0) remaining = 0.0;
            }
 
            struct timeval tv;
            tv.tv_sec  = (long)remaining;
            tv.tv_usec = (long)((remaining - (long)remaining) * 1000000L);
 
            fd_set rfds;
            FD_ZERO(&rfds);
            FD_SET(sock, &rfds);
 
            int sel = select(0, &rfds, NULL, NULL, &tv);
            if (sel == SOCKET_ERROR) {
                fprintf(stderr, "select() fout: %d\n", WSAGetLastError());
                break;
            }
 
            if (sel > 0) {
                char buf[BUF_SIZE];
                struct sockaddr_in client_addr;
                int addr_len = sizeof(client_addr);
 
                int n = recvfrom(sock, buf, BUF_SIZE - 1, 0,
                                 (struct sockaddr *)&client_addr, &addr_len);
                if (n == SOCKET_ERROR) continue;
                buf[n] = '\0';
 
                char *nl = strchr(buf, '\n'); if (nl) *nl = '\0';
                nl = strchr(buf, '\r');       if (nl) *nl = '\0';
 
                if (phase == 0) {
                    char *end;
                    long guess = strtol(buf, &end, 10);
                    if (end == buf || *end != '\0') {
                        printf("  [WARN] Ongeldige invoer van %s:%d: '%s'\n",
                               inet_ntoa(client_addr.sin_addr),
                               ntohs(client_addr.sin_port), buf);
                        continue;
                    }
 
                    int diff = abs((int)guess - secret);
                    printf("  Gok van %s:%d: %ld  (verschil: %d)\n",
                           inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port), guess, diff);
 
                    int found = 0;
                    for (int i = 0; i < player_count; i++) {
                        if (addr_eq(&players[i].addr, &client_addr)) {
                            players[i].last_guess = (int)guess;
                            found = 1; break;
                        }
                    }
                    if (!found && player_count < MAX_PLAYERS) {
                        players[player_count].addr       = client_addr;
                        players[player_count].last_guess = (int)guess;
                        player_count++;
                    }
 
                    if (diff < closest_diff) {
                        closest_diff  = diff;
                        closest_value = (int)guess;
                        closest_addr  = client_addr;
                        has_winner    = 1;
                        printf("  >> Nieuwe dichtstbijzijnde: %s:%d met %ld\n",
                               inet_ntoa(client_addr.sin_addr),
                               ntohs(client_addr.sin_port), guess);
                    }
 
                    double cur = now_sec();
                    if (timeout_at == 0.0) {
                        timeout_at = cur + INITIAL_TIMEOUT;
                        printf("  Timer gestart: %.1fs\n", INITIAL_TIMEOUT);
                    } else {
                        double rem     = timeout_at - cur;
                        double new_rem = (rem / 2.0 < 0.5) ? 0.5 : rem / 2.0;
                        timeout_at     = cur + new_rem;
                        printf("  Timer gehalveerd: nog %.2fs\n", new_rem);
                    }
 
                } else {
                    printf("  Bericht na time-out van %s:%d -> 'You lost !'\n",
                           inet_ntoa(client_addr.sin_addr),
                           ntohs(client_addr.sin_port));
                    send_msg(sock, "You lost !", &client_addr);
                }
            }
 
            if (timeout_at != 0.0 && now_sec() >= timeout_at) {
                if (phase == 0) {
                    if (!has_winner) {
                        printf("  Time-out maar geen spelers. Wacht opnieuw...\n");
                        timeout_at = 0.0;
                        continue;
                    }
                    printf("\n  TIME-OUT! Winnaar: %s:%d  gok=%d  geheim=%d\n",
                           inet_ntoa(closest_addr.sin_addr),
                           ntohs(closest_addr.sin_port),
                           closest_value, secret);
                    send_msg(sock, "You won ?", &closest_addr);
                    printf("  'You won ?' gestuurd. Wacht %.0fs op bevestiging...\n",
                           CONFIRM_TIMEOUT);
                    timeout_at = now_sec() + CONFIRM_TIMEOUT;
                    phase = 1;
                } else {
                    printf("  Geen bevestiging. Stuur automatisch 'You won !'\n");
                    send_msg(sock, "You won !", &closest_addr);
                    break;
                }
            }
        }
 
        printf("\n  Spel afgelopen. Geheim was %d.\n", secret);
        Sleep(2000);
    }
 
    closesocket(sock);
    WSACleanup();
    return 0;
}