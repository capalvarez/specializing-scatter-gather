#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <signal.h>
#include <thread>
#include <cmath>
#include <chrono>
#include <fcntl.h>

#include "base_worker.h"
#include "config/background_config.h"

/*
 *
 */
void serve(int sock, BaseConfig* baseConfig){
    BackgroundConfig* config = (BackgroundConfig*) baseConfig;

    ssize_t total_bytes_written = 0;

    int64_t index, number_workers, payload_size;
    receive_int(&index, sock);
    receive_int(&number_workers, sock);
    receive_int(&payload_size, sock);

    while(total_bytes_written < config->bytes_to_send){
        ssize_t bytes_written = write(sock, &config->buffer[total_bytes_written], config->bytes_to_send - total_bytes_written);
        total_bytes_written += bytes_written;
    }
}


int main(int argc, char* argv[]) {
    if(argc < 2){
        perror("Parameters: config_file");
        exit(EXIT_FAILURE);
    }

    std::string config_file = argv[1];
    BackgroundConfig* config = new BackgroundConfig();
    config->load_config(config_file);

    base_worker_loop(config, serve);

}