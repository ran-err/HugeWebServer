#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

int main (int argc, char *argv[]) {
  if (argc <= 2) {
    printf("Usage: %s ip_address portname\n", argv[0]);
    return 0;
  }

  const char *ip = argv[1];
  int port = atoi(argv[2]);

  int listenfd = socket(PF_INET, SOCK_STREAM, 0);  // IPv4, TCP
  assert(listenfd >= 0);

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;  // address family
  address.sin_port = htons(port);  // host to network short
  inet_pton(AF_INET, ip, &address.sin_addr);  // dotted decimal string to struct

  int ret = 0;
  ret = bind(listenfd, (struct sockaddr*)(&address), sizeof(address));
  assert(ret != -1);
  ret = listen(listenfd, 5);
  assert(ret != -1);

  struct sockaddr_in client;
  socklen_t client_addrlength = sizeof(client);
  int sockfd = accept(listenfd, (struct sockaddr*)(&address), &client_addrlength);

  constexpr int buf_size = 10;
  char buffer[buf_size] = {0};
  int recv_size = 0;
  recv_size = recv(sockfd, buffer, buf_size, 0);

  int send_size = 0;
  send_size = send(sockfd, buffer, recv_size, 0);

  close(sockfd);
  close(listenfd);

  return 0;
}
