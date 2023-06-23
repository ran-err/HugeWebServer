#include <cassert>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
// system api
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

namespace Utils {
  int setnonblocking(int fd) {
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
      fcntl(fd, F_SETFL, new_option);
      return old_option;
  }
  void addfd(int epollfd, int fd, bool enable_et = true) {
      epoll_event event;
      event.events = EPOLLIN | EPOLLET;
      event.data.fd = fd;
      epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
      setnonblocking(fd);
  }
}

namespace Epoll {
    void et(epoll_event *events, int number, int epollfd, int listenfd) {
        constexpr int BUFFER_SIZE = 10;
        char buffer[BUFFER_SIZE];
        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;
            if (sockfd == listenfd) {  // new connection
                struct sockaddr_in client_address;
                socklen_t client_addrlength = sizeof(client_address);
                int connfd = accept(listenfd, (struct sockaddr*)(&client_address), &client_addrlength);
                Utils::addfd(epollfd, connfd);
                char ip[1024];
                inet_ntop(AF_INET, &client_address.sin_addr, ip, 1024);
                printf("new connection from %s\n", ip);
            } else if (events[i].events & EPOLLIN) {
                printf("EPOLLIN event trigger once\n");
                while (true) {  // read data
                    memset(buffer, 0, BUFFER_SIZE);
                    int recv_size = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
                    if (recv_size < 0) {  // read error
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {  // read completed
                            printf("read later\n");
                            break;
                        }
                        printf("sockfd %d, recv msg failed\n", sockfd);
                        close(sockfd);
                        break;
                    } else if (recv_size == 0) {  // read nothing
                        close(sockfd);
                    } else {  // got data
                        send(sockfd, buffer, recv_size, 0);
                        printf("sending data\n");
                    }
                }
            }
        }
    }
}

int main (int argc, char *argv[]) {
  if (argc <= 2) {
    printf("Usage: %s ip_address portname\n", argv[0]);
    return 0;
  }

  const char *ip = argv[1];
  int port = atoi(argv[2]);

  int listenfd = socket(PF_INET, SOCK_STREAM, 0);  // IPv4, TCP
  assert(listenfd >= 0);

  struct sockaddr_in address;  // host address
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;  // address family
  address.sin_port = htons(port);  // host to network short
  inet_pton(AF_INET, ip, &address.sin_addr);  // dotted decimal string to struct

  int ret = 0;
  ret = bind(listenfd, (struct sockaddr*)(&address), sizeof(address));
  assert(ret != -1);
  ret = listen(listenfd, 5);
  assert(ret != -1);

  constexpr int MAX_EVENT_NUMBER = 5;
  epoll_event events[MAX_EVENT_NUMBER];  // array for reading epoll events
  int epollfd = epoll_create(1);  // size is a hint for kernel event table size
  assert(epollfd != -1);
  Utils::addfd(epollfd, listenfd);

  while (true) {
      int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
      if (ret < 0) {
          printf("epoll faliure\n");
          break;
      }
      Epoll::et(events, ret, epollfd, listenfd);
  }

  close(listenfd);
  return 0;

//  struct sockaddr_in client;
//  socklen_t client_addrlength = sizeof(client);
//  int sockfd = accept(listenfd, (struct sockaddr*)(&address), &client_addrlength);
//
//  constexpr int buf_size = 10;
//  char buffer[buf_size] = {0};
//  int recv_size = 0;
//  recv_size = recv(sockfd, buffer, buf_size, 0);
//
//  int send_size = 0;
//  send_size = send(sockfd, buffer, recv_size, 0);
//
//  close(sockfd);
//  close(listenfd);

  return 0;
}
