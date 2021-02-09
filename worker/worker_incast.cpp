#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <signal.h>
#include <thread>

#include "config/base_config.h"
#include "base_worker.h"

/*
 * When a request is received, read the payload size from the socket and answer with the required number of bytes.
 */
void serve(int sock, BaseConfig* baseConfig){
    ssize_t total_bytes_written = 0;
    int64_t payload_size;

    receive_int(&payload_size, sock);

    while (total_bytes_written < payload_size) {
        ssize_t bytes_written = write(sock, &baseConfig->buffer[total_bytes_written], payload_size - total_bytes_written);
        if (bytes_written == -1) {
            return;
        }

        total_bytes_written += bytes_written;
    }    
}

int main(int argc, char* argv[]) {
    if(argc < 2){
         perror("Parameters: path_to_config_file");
        exit(EXIT_FAILURE);
    }

    std::string config_file = argv[1];
    BaseConfig* baseConfig = new BaseConfig();
    baseConfig->load_config(config_file);

    base_worker_loop(baseConfig, serve);
}