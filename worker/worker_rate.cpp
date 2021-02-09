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
#include "config/rate_config.h"

struct ScheduleInfo{
    int to_send;
    int time;

    ScheduleInfo(){
        to_send = 0;
        time = 0;
    }
};

void serve(int sock, BaseConfig* baseConfig){
    RateConfig* config = (RateConfig*) baseConfig;
    
    ssize_t total_bytes_written = 0;

    std::chrono::high_resolution_clock::time_point startTime = std::chrono::high_resolution_clock::now();

    bool             res;
    fd_set          sready;
    struct timeval  nowait;

    FD_ZERO(&sready);
    FD_SET((unsigned int)sock, &sready);

    memset((char *)&nowait,0,sizeof(nowait));

    int64_t index, number_workers, payload_size;
    receive_int(&index, sock);
    receive_int(&number_workers, sock);
    receive_int(&payload_size, sock);

    std::chrono::high_resolution_clock::time_point got_data = std::chrono::high_resolution_clock::now();

    // Create schedule
    int n = ceil((double) payload_size / config->chunk_size_per_round);

    int standard_transmission = ceil(((double) 8 * config->chunk_size_per_round / config->bandwidth) * (1 + config->safety_factor));

    std::vector<ScheduleInfo> my_schedule;

    int current_round = 0;

    int accumulated_bytes = 0;

    for (int round = 0; round < n-1; ++round) {
        ScheduleInfo scheduleInfo;
        scheduleInfo.time = index * standard_transmission + round * number_workers * (standard_transmission + config->inter_request);
        scheduleInfo.to_send = config->chunk_size_per_round;

        accumulated_bytes += scheduleInfo.to_send;
        my_schedule.emplace_back(scheduleInfo);
    }

    int expected_ending = (n-1) * (standard_transmission + config->inter_request) * number_workers;

    int64_t leftover = payload_size - accumulated_bytes;
    if(leftover){
        int leftover_transmission = ceil((double) 8 * leftover / config->MSS * config->MTU / config->bandwidth) * (1 + config->safety_factor);

        ScheduleInfo scheduleInfo;
        scheduleInfo.time = (n - 1) * number_workers * (standard_transmission + config->inter_request) + (leftover_transmission + config->inter_request) * index;
        scheduleInfo.to_send = leftover;

        my_schedule.emplace_back(scheduleInfo);

        expected_ending += number_workers * (leftover_transmission + config->inter_request);
    }

    int objective_round = 0;

    std::chrono::high_resolution_clock::time_point ended_processing = std::chrono::high_resolution_clock::now();

    while (total_bytes_written < payload_size) {
        // Wait to receive the cancelling message
        int time_to_wait;

        if(current_round){
            time_to_wait = my_schedule[current_round].time - my_schedule[current_round - 1].time;
        } else{
            time_to_wait = my_schedule[current_round].time - std::chrono::duration_cast<std::chrono::microseconds>(ended_processing - got_data).count();

            if(time_to_wait < 0){
                time_to_wait = 0;
            }
        }

        int status = -1;
        int64_t number_cancelled = 0;

        if(time_to_wait){
            nowait.tv_sec = 0;
            nowait.tv_usec = time_to_wait;

            // If a cancelling message was received, we need to use the number included in the message to compute the next slot
            res = select(sock + 1,&sready,NULL,NULL,&nowait);
            if(FD_ISSET(sock,&sready)){
                status = 1;
                receive_int(&number_cancelled, sock);
            }
        }

        if (status < 0) {
            // Timeout, current sending was not cancelled
            objective_round += my_schedule[current_round].to_send;

            while(total_bytes_written < objective_round){
                ssize_t bytes_written = write(sock, &config->buffer[total_bytes_written], objective_round - total_bytes_written);
                total_bytes_written += bytes_written;
            }

            current_round++;
        } else{
            int new_time = expected_ending + standard_transmission * number_cancelled;

            ScheduleInfo scheduleInfo;
            scheduleInfo.time = new_time;
            scheduleInfo.to_send = my_schedule[current_round].to_send;

            my_schedule.emplace_back(scheduleInfo);
            current_round++;
        }
    }
}


int main(int argc, char* argv[]) {
    if(argc < 2){
        perror("Parameters: config_file");
        exit(EXIT_FAILURE);
    }

    std::string config_file = argv[1];
    RateConfig* config = new RateConfig(); 
    config->load_config(config_file);

    base_worker_loop(config, serve);
}