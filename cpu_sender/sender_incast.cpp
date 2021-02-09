#include <iostream>
#include <stdio.h>
#include <vector>
#include <algorithm>
#include <chrono>
#include <future>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unordered_map>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/thread.h>
#include <event.h>
#include <cstring>
#include <fstream>
#include <string>
#include <math.h>
#include <dirent.h>
#include <signal.h>

#include "utils/ThreadPool.h"
#include "basic_sender.h"

/*
 * Send requests to workers in groups of 2 to avoid request synchronization.
 */
void sending_loop(ExperimentInfo* info, ThreadInfo* threadInfo){
    {
        std::unique_lock<std::mutex> lock(info->sendMutex);

        for (int j = 0; j < 2; ++j) {
            client* newClient;

            if(threadInfo->requestQueue.empty()){
                return;
            }

            newClient = threadInfo->requestQueue.front();
            threadInfo->requestQueue.pop();

            std::vector<int64_t> to_send {info->CHUNK_SIZE};
            request_to_send(threadInfo, newClient, to_send);
        }
    }
}

/*
 * Standard libevent callback: called when something has been sent through the socket. It allows the thread to
 * continue sending.
 */
void on_write(struct bufferevent *bev, void *arg){
    CallbackInfo *info = (CallbackInfo*)arg;

    register_write(info);
    sending_loop(info->info, info->threadInfo);
}

/*
 * Called after a reply has been received and it's yet to be processed.
 */
void reply_preprocess(CallbackInfo* info){
    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    info->threadInfo->receivingTimes.insert(std::make_pair(info->client_info->index, now));
}

/*
 * Called after a reply has been processed. It checks whether all replies has been received to
 * exit the libevent loop.
 */
void reply_postprocess(CallbackInfo* info){
    int received = info->threadInfo->receivingTimes.size();

    if (info->client_info->expectedConnected <= received) {
        {
            std::unique_lock<std::mutex> lock(info->info->mutex);

            info->info->condition.notify_all();
            event_base_loopbreak(info->client_info->evbase);
        }

    }
}

/*
 * Standard libevent callback: called when there is something to be read from the socket.
 */
void on_read(struct bufferevent *bev, void *arg){
    CallbackInfo *info = (CallbackInfo*)arg;

    handle_reply(bev, info, reply_preprocess, reply_postprocess);
}

/*
 * Callback used to communicate between threads. In this case, it does nothing.
 */
void available_token_notification(evutil_socket_t ev, short what, void *arg){}

/*
 * Creates the connection to each worker and sets the libevent callbacks as required.
 */
void setup_connection(CallbackInfo* info){
    create_connection(info->threadInfo, info->client_info);
    bufferevent_setcb(info->client_info->buf_ev, on_read, on_write, on_connection, info);
}

/*
 * Starts a thread running the incast sender.
 */
void start_thread_incast(ExperimentInfo* info, int startIndex, int threadIndex, int notifyPipe, ThreadInfo* threadInfo){
    start_thread(info, startIndex, threadIndex, notifyPipe, threadInfo, sending_loop, setup_connection,
                 available_token_notification);
}

int main(int argc, char* argv[]) {
    if (argc < 4){
        perror("Parameters: chunk_size number_threads config_file");
        exit(EXIT_FAILURE);
    }

    ExperimentInfo* info = initialize_basic_sender(argv);
    run_experiment(info, start_thread_incast);
}
