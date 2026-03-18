#include <_stdio.h>
#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/_endian.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static_assert(true, "header to fix bug causing clangd pragma warning");

#pragma pack(push, 1)
// read in reverse order since we have to flip big endian
typedef struct dns_flags {
  uint16_t rcode : 4;
  uint16_t cd : 1;
  uint16_t ad : 1;
  uint16_t z : 1;
  uint16_t ra : 1;
  uint16_t rd : 1;
  uint16_t tc : 1;
  uint16_t aa : 1;
  uint16_t opcode : 4;
  uint16_t qr : 1;
} dns_flags_t;

typedef struct dns_header {
  uint16_t id;
  dns_flags_t flags;
  uint16_t qdcount;
  uint16_t ancount;
  uint16_t nscount;
  uint16_t arcount;
} dns_header_t;

#pragma pack(pop)

int sockfd;

void parse_header(char *buffer) {
  dns_header_t *header = (dns_header_t *)buffer;
  uint16_t id = ntohs(header->id);

  uint16_t raw_flags = ntohs(*(uint16_t *)&header->flags);
  dns_flags_t *flags = (dns_flags_t *)&raw_flags;

  // extracting with bitshift & mask
  uint8_t qr = (raw_flags >> 15) & 0x1;
  uint8_t op = (raw_flags >> 11) & 0xF;
  uint8_t aa = (raw_flags >> 10) & 0x1;
  uint8_t tc = (raw_flags >> 9) & 0x1;
  uint8_t rd = (raw_flags >> 8) & 0x1;
  uint8_t ra = (raw_flags >> 7) & 0x1;
  uint8_t z = (raw_flags >> 6) & 0x1;
  uint8_t ad = (raw_flags >> 5) & 0x1;
  uint8_t cd = (raw_flags >> 4) & 0x1;
  uint8_t rcode = raw_flags & 0xF;

  printf("id: %d\n", id);
  printf("qr: %d\n", qr);
  printf("aa: %d\n", aa);
  printf("tc: %d\n", tc);
  printf("rd: %d\n", rd);
  printf("ra: %d\n", ra);
  printf("z: %d\n", z);
  printf("ad: %d\n", ad);
  printf("cd: %d\n", cd);
  printf("rcode: %d\n", rcode);

  // easier version
  printf("id: %d\n", id);
  printf("qr: %d\n", flags->qr);
  printf("aa: %d\n", flags->aa);
  printf("tc: %d\n", flags->tc);
  printf("rd: %d\n", flags->rd);
  printf("ra: %d\n", flags->ra);
  printf("z: %d\n", flags->z);
  printf("ad: %d\n", flags->ad);
  printf("cd: %d\n", flags->cd);
  printf("rcode: %d\n", flags->rcode);

  // other headers
  uint16_t qdcount = ntohs(header->qdcount);
  uint16_t ancount = ntohs(header->ancount);
  uint16_t nscount = ntohs(header->nscount);
  uint16_t arcount = ntohs(header->arcount);

  printf("qdcount %d\n", qdcount);
  printf("ancount %d\n", ancount);
  printf("nscount %d\n", nscount);
  printf("arcount %d\n", arcount);

  uint8_t *qname = (uint8_t *)buffer + sizeof(dns_header_t);

  while (*qname != 0) {
    qname += *qname + 1;
  }
  printf("break");
}

void sig_handler(int signum) {
  printf("\nCaught signal, exiting, %d", signum);
  close(sockfd);
  exit(0);
}

int main() {
  char *ip = "127.0.0.1";
  int port = 9053;

  struct sockaddr_in server_addr, client_addr;
  char buffer[1024];
  socklen_t addr_size;
  int n;

  // kill this thing dead
  struct sigaction sa;
  sa.sa_handler = sig_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);
  sigaction(SIGHUP, &sa, NULL);
  sigaction(SIGQUIT, &sa, NULL);

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
  close(sockfd);
  return 0;
}
