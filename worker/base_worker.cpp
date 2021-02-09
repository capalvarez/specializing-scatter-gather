#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <signal.h>
#include <thread>

#include "base_worker.h"

int64_t receive_int(int64_t *num, int fd) {
    int64_t ret;
    char *data = (char*)&ret;
    int left = sizeof(ret);
    int rc;
    do {
        rc = read(fd, data, left);
        if (rc <= 0) { /* instead of ret */
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {

            }
            else if (errno != EINTR) {
                return -1;
            }
        }
        else {
            data += rc;
            left -= rc;
        }
    }
    while (left > 0);
    *num = ret;
    return 1;
}


void base_worker_loop(BaseConfig* baseConfig, void (*serve)(int,BaseConfig*)){
    int fd;
    struct sockaddr_in address;
    int address_len;

    /* Pin process to cpu */
    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    CPU_SET(baseConfig->cpu_index, &my_set);
    sched_setaffinity(0, sizeof(cpu_set_t), &my_set);
    address_len = sizeof(baseConfig->socket_address);

    if((fd = socket(AF_INET, SOCK_STREAM, 0)) == 0){
        perror("Can not create the socket");
        exit(EXIT_FAILURE);
    }

    int reuse_port = 1, reuse_addr = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &reuse_port, sizeof(reuse_port));
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));

    address.sin_port = htons(baseConfig->port);
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;

    if(bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0){
        perror("Can not bind to port");
        exit(EXIT_FAILURE);
    }

    if(listen(fd, 1) < 0){
        perror("Listening failed");
        exit(EXIT_FAILURE);
    }

    while(1){
        int sock;

        if((sock = accept(fd, (struct sockaddr*) &address, (socklen_t*) &address_len)) < 0){
            perror("Can not accept connection");
            exit(EXIT_FAILURE);
        }

        std::thread thread(serve, sock, baseConfig);
        thread.detach();
    }
}


