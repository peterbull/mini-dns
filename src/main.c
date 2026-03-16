#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdbool.h>

static_assert(true, "header to fix bug causing clangd pragma warning");

#pragma pack(push, 1)
typedef struct dns_header {
  uint16_t id;
  uint16_t flags;
} dns_header_t;
#pragma pack(pop)

void parse_header(char *buffer) {
  dns_header_t *header = (dns_header_t *)buffer;
  uint16_t id = ntohs(header->id);
  uint16_t flags = ntohs(header->flags);
  uint8_t qr = (flags >> 15) & 0x1; 
  printf("id: %d\n", id);
  printf("qr: %d\n", qr);
}

void sig_handler(int signum) {
  printf("\nCaught signal, exiting, %d", signum);
  exit(0);
}

int main() {
  char *ip = "127.0.0.1";
  int port = 9053;

  int sockfd;
  struct sockaddr_in server_addr, client_addr;
  char buffer[1024];
  socklen_t addr_size;
  int n;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  memset(&server_addr, '\0', sizeof(server_addr));
  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(port);
  server_addr.sin_addr.s_addr = inet_addr(ip);

  n = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
  if (n < 0) {
    perror("[-]bind error");
    exit(1);
  }

  signal(SIGINT, sig_handler);

  while (1) {
    memset(buffer, '\0', 1024);
    addr_size = sizeof(client_addr);
    recvfrom(sockfd, buffer, 1024, 0, (struct sockaddr *)&client_addr,
             &addr_size);
    parse_header(buffer);
    memset(buffer, '\0', 1024);
    strcpy(buffer, "dns server running");
    sendto(sockfd, buffer, 1024, 0, (struct sockaddr *)&client_addr,
           sizeof(client_addr));
    printf("[+]data send: %s\n ", buffer);
  }
  return 0;
}
