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
#include <map>
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
#include "utils/rate-control-info.h"
#include "utils/string_utils.h"

RateControlInfo rateControlInfo;
std::mutex rateControlMutex;
std::mutex cancellingMutex;

/** FUNCTION DECLARATIONS **/
void put_cancelling_in_buffer(client *client_info, int cancelling_count);


/** GENERAL EXPERIMENT FUNCTIONS **/
/*
 * Send requests to workers, desynchronization is not necessary as the workers will desynchronize themselves.
 */
void sending_loop(ExperimentInfo* info, ThreadInfo* threadInfo){
    int to_send = threadInfo->requestQueue.size();

    for (int j = 0; j < to_send; ++j) {
        client* newClient;

        newClient = threadInfo->requestQueue.front();
        threadInfo->requestQueue.pop();

        std::vector<int64_t> to_send {newClient->index, info->NUMBER_WORKERS, info->CHUNK_SIZE};
        request_to_send(threadInfo, newClient, to_send);
    }
}

/*
 * Standard libevent callback: called when something has been sent through the socket. It registers that the request has
 * been sent.
 */
void on_write(struct bufferevent *bev, void *arg){
    CallbackInfo *info = (CallbackInfo*)arg;
    register_write(info);
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
void on_read(struct bufferevent *bev, void *arg) {
    CallbackInfo *info = (CallbackInfo *) arg;

    char buffer[info->info->CHUNK_SIZE];
    int bytesRead = 0;
    int result;

    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

    std::unique_lock<std::mutex> lock(rateControlMutex, std::defer_lock);

    if (lock.try_lock()) {
        if (!rateControlInfo.received_first_byte) {
            rateControlInfo.previous_time = std::chrono::high_resolution_clock::now();
        }

        lock.unlock();
    }

    int accumulated_bytes = 0;

    while ((bytesRead = EVBUFFER_LENGTH(bev->input)) > 0 && info->client_info->previousBytes < info->info->CHUNK_SIZE) {
        if (info->client_info->previousBytes == 0) {
            info->threadInfo->firstByteTimes.insert(std::make_pair(info->client_info->index, now));
        }

        if (bytesRead > 4096) {
            bytesRead = 4096;
        }

        result = evbuffer_remove(bev->input, buffer + info->client_info->previousBytes,
                                 info->info->CHUNK_SIZE > bytesRead ? info->info->CHUNK_SIZE - bytesRead : bytesRead);
        info->client_info->previousBytes += result;
        accumulated_bytes += result;
    }

    {
        std::unique_lock<std::mutex> lock(rateControlMutex);

        rateControlInfo.accumulated_bytes += accumulated_bytes;

        int round = floor((double) info->client_info->previousBytes / 13800);

        for (int i = 0; i < round; i++) {
            int current_event_worker = rateControlInfo.events_per_worker[info->client_info->index][i];

            if (!rateControlInfo.scheduled_events[current_event_worker].completed) {
                rateControlInfo.scheduled_events[current_event_worker].completed = true;
            }
        }

        int received_event;

        for (int i = 0; i < (int) rateControlInfo.events_per_worker[info->client_info->index].size(); i++) {
            if (!rateControlInfo.scheduled_events[rateControlInfo.events_per_worker[info->client_info->index][i]].completed) {
                received_event = rateControlInfo.events_per_worker[info->client_info->index][i];
                break;
            }
        }

        if (received_event > rateControlInfo.current_event) {
            rateControlInfo.current_event = received_event;
        }

        int elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(
                now - rateControlInfo.previous_time).count();

        if (elapsed_time >= rateControlInfo.reference_window) {
            int64_t delta = std::chrono::duration_cast<std::chrono::microseconds>(
                    now - rateControlInfo.previous_time).count();

            double rate = rateControlInfo.accumulated_bytes * 8.0 / delta;

            if (rate < (1 - rateControlInfo.rate_tolerance) *
                       rateControlInfo.rate_per_round[rateControlInfo.scheduled_events[rateControlInfo.current_event].round]) {

                rateControlInfo.should_cancel ++;

                rateControlInfo.previous_time = now;
                rateControlInfo.accumulated_bytes = 0;

                {
                    std::unique_lock<std::mutex> threads_lock(info->info->mutex);
                    notify_all_threads(info->info);
                }
            }
        }
    }

    if (info->client_info->previousBytes >= info->info->CHUNK_SIZE) {
        reply_preprocess(info);

        info->threadInfo->received.insert(info->client_info->index);

        reply_postprocess(info);

        int received_thread = 0;
        int expected = info->threadInfo->expected;

        received_thread = info->threadInfo->received.size();

        {
            std::unique_lock<std::mutex> lock(info->info->mutex);
            notify_all_threads(info->info);
        }

        if (received_thread == expected) {
            event_base_loopbreak(info->client_info->evbase);
        }
    }
}

/*
 * Callback used to communicate between threads. In this case, it is used to notify a thread that it should send a
 * cancelling packet.
 */
void available_token_notification(evutil_socket_t ev, short what, void *arg){
    NotifyInfo* info = (NotifyInfo*) arg;

    char buf[1];

    if (read(ev, buf, 1) != 1) {
        fprintf(stderr, "Can't read from libevent pipe\n");
        return;
    }

    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

    std::unique_lock<std::mutex> lock(cancellingMutex, std::defer_lock);

    if (lock.try_lock()) {
        {
            std::unique_lock<std::mutex> rate_lock(rateControlMutex);
            if(!rateControlInfo.should_cancel) {
                return;
            }

            rateControlInfo.should_cancel--;
        }

        int number_events = (int) rateControlInfo.scheduled_events.size();

        int idx = rateControlInfo.current_event + rateControlInfo.cancel_distance;

        while(idx < number_events &&
        info->threadInfo->my_workers.find(rateControlInfo.scheduled_events[idx].worker_index) == info->threadInfo->my_workers.end()){
            idx++;
        }

        if (idx < number_events){
            rateControlInfo.scheduled_events[idx].status = false;

            ScheduledEvent scheduledEvent;
            scheduledEvent.delay = rateControlInfo.accumulated_delay;
            scheduledEvent.bytes = rateControlInfo.scheduled_events[idx].bytes;
            scheduledEvent.worker_index = rateControlInfo.scheduled_events[idx].worker_index;
            scheduledEvent.round = rateControlInfo.scheduled_events[idx].round;

            rateControlInfo.accumulated_delay += ceil(((double) 8 * rateControlInfo.scheduled_events[idx].bytes / rateControlInfo.mss * rateControlInfo.mtu  / rateControlInfo.bandwidth) * (1 + rateControlInfo.safety_factor));

            rateControlInfo.scheduled_events.emplace_back(scheduledEvent);

            std::vector<int64_t> to_send (rateControlInfo.cancelling_count);
            put_request_in_buffer(info->threadInfo->connections_information[rateControlInfo.scheduled_events[idx].worker_index], to_send);

            rateControlInfo.cancelling_count++;
        }else{
            {
                std::unique_lock<std::mutex> rate_lock(rateControlMutex);
                rateControlInfo.should_cancel++;
            }
        }

        {
            std::unique_lock<std::mutex> lock(info->info->mutex);
            notify_all_threads(info->info);
        }

        lock.unlock();
    }
}

/*
 * Creates the connection to each worker and sets the libevent callbacks as required.
 */
void setup_connection(CallbackInfo* info){
    create_connection(info->threadInfo, info->client_info);

    bufferevent_setcb(info->client_info->buf_ev, on_read, on_write, on_connection, info);
}

void start_thread_rate(ExperimentInfo* info, int startIndex, int threadIndex, int notifyPipe, ThreadInfo* threadInfo){
    start_thread(info, startIndex, threadIndex, notifyPipe, threadInfo, sending_loop, setup_connection,
                 available_token_notification);
}

/** RATE CONTROL FUNCTIONS **/
/*
 * Computes the schedule based on the experiment parameters.
 */
void compute_schedule(ExperimentInfo* info, std::string config_file){
    std::map<std::string, std::string> config;
    read_config_file(config_file, config);

    int mss = parse_int64(get_param("mss", config));
    int mtu = parse_int64(get_param("mtu", config));
    int init_cwnd = parse_int64(get_param("init_cwnd", config));
    int bandwidth = parse_int64(get_param("bandwidth", config));

    double safety_factor = parse_double(get_param("safety_factor", config));
    double rate_tolerance = parse_double(get_param("rate_tolerance", config));

    int reference_window = parse_int64(get_param("reference_window", config));
    int size_per_round = parse_int64(get_param("size_per_round", config));

    int cancel_distance = parse_int64(get_param("cancel_distance", config));

    int n = ceil((double) info->CHUNK_SIZE / (size_per_round));

    rateControlInfo.init_cwnd = init_cwnd;
    rateControlInfo.mss = mss;
    rateControlInfo.mtu = mtu;
    rateControlInfo.size_per_round = size_per_round;
    rateControlInfo.bandwidth = bandwidth;

    rateControlInfo.reference_window = reference_window;
    rateControlInfo.rate_tolerance = rate_tolerance;
    rateControlInfo.safety_factor = safety_factor;

    rateControlInfo.cancel_distance = cancel_distance;

    rateControlInfo.events_per_worker = std::vector<std::vector<int>>(info->NUMBER_WORKERS, std::vector<int>());

    double accumulated_bytes = 0;
    int accumulated_delay = 0;

    for (int round = 0; round < n - 1; ++round) {
        int transmission_time = ceil((double) 8 * size_per_round / bandwidth);

        for (int idx = 0; idx < (int) info->NUMBER_WORKERS; idx++) {
            ScheduledEvent scheduledEvent;
            scheduledEvent.delay = accumulated_delay;
            scheduledEvent.bytes = size_per_round;
            scheduledEvent.worker_index = idx;
            scheduledEvent.round = round;

            accumulated_delay += ceil(transmission_time * (1 + safety_factor));

            rateControlInfo.events_per_worker[idx].emplace_back(rateControlInfo.scheduled_events.size());
            rateControlInfo.scheduled_events.emplace_back(scheduledEvent);
        }

        accumulated_bytes += size_per_round;

        rateControlInfo.rate_per_round.emplace_back(((double) 8 * init_cwnd * mss) / (transmission_time * (1 + safety_factor)));
    }

    int64_t leftover = info->CHUNK_SIZE - accumulated_bytes;

    if(leftover){
        int to_send_round = leftover;
        int transmission_time = ceil((double) 8 * leftover / mss * mtu  / bandwidth);

        for (int idx = 0; idx < (int) info->NUMBER_WORKERS; idx++) {
            ScheduledEvent scheduledEvent;
            scheduledEvent.delay = accumulated_delay;
            scheduledEvent.bytes = to_send_round;
            scheduledEvent.worker_index = idx;
            scheduledEvent.round = n-1;

            accumulated_delay += ceil(transmission_time * (1 + safety_factor));

            rateControlInfo.events_per_worker[idx].emplace_back(rateControlInfo.scheduled_events.size());
            rateControlInfo.scheduled_events.emplace_back(scheduledEvent);
        }

        rateControlInfo.rate_per_round.emplace_back(((double) 8 * leftover / mss * mtu) / (transmission_time * (1 + safety_factor)));
    }

    rateControlInfo.accumulated_delay = accumulated_delay;
}


/** BACKGROUND TRAFFIC FUNCTIONS **/
/*
 * Start the thread that will send the requests to the workers that act as background traffic.
 */
void start_background_thread(ExperimentInfo* info, BackgroundTrafficThreadInfo* backgroundInfo,
                             std::vector<IpPort> background);

/*
 * Initialize information for background traffic and start the dedicated thread.
 */
void run_background(ExperimentInfo* info, ThreadPool* pool, BackgroundTrafficThreadInfo* backgroundInfo,
        std::vector<IpPort> backgroundWorkers){
    pool->enqueue([info,backgroundInfo,backgroundWorkers]{
        return start_background_thread(info, backgroundInfo, backgroundWorkers);});
}

/*
 * Standard libevent callback. For background traffic: read as much information as possible from the corresponding
 * socket, exit the loop if all the information expected has been received.
 */
void on_read_background(struct bufferevent *bev, void *arg){
    BackgroundCallbackInfo *info = (BackgroundCallbackInfo *) arg;

    char buffer[4096];
    int bytesRead = 0;
    int result = 0;

    while ((bytesRead = EVBUFFER_LENGTH(bev->input)) > 0) {
        if (bytesRead > 4096) {
            bytesRead = 4096;
        }

        result = evbuffer_remove(bev->input, buffer,
                                 info->backgroundInfo->total_bytes > bytesRead ? info->backgroundInfo->total_bytes - bytesRead : bytesRead);
        info->client_info->previousBytes += result;
    }

    if(info->client_info->previousBytes >= info->backgroundInfo->total_bytes){
        info->backgroundInfo->fake_workers_finished++;

        if(info->backgroundInfo->fake_workers_finished == info->client_info->expectedConnected){
            event_base_loopbreak(info->client_info->evbase);
        }
    }
}

/*
 * Standard libevent callback.
 */
void on_write_background(struct bufferevent *bev, void *arg){}

/*
 * Standard libevent callback. For background traffic: register that a worker has finish connecting and exit the loop if
 * all of them are ready.
 */
void on_connection_background(struct bufferevent *bev, short what, void *arg) {
    BackgroundCallbackInfo* callbackInfo = (BackgroundCallbackInfo*) arg;

    {
        std::unique_lock<std::mutex> lock(callbackInfo->info->mutex);

        callbackInfo->backgroundInfo->connectedCounter++;

        if (callbackInfo->client_info->expectedConnected <= callbackInfo->backgroundInfo->connectedCounter){
            callbackInfo->info->connectedBackground = true;
            callbackInfo->info->condition.notify_all();

            event_base_loopbreak(callbackInfo->client_info->evbase);
        }
    }
}

/*
 * For background traffic: creates the connection to each worker and sets the libevent callbacks as required.
 */
void create_background_connection(CallbackInfo* info){
    create_connection(info->threadInfo, info->client_info);
    bufferevent_setcb(info->client_info->buf_ev, on_read_background, on_write_background, on_connection_background, info);
}

/*
 * Send requests to background workers.
 */
void sending_loop_background(ExperimentInfo* info, BackgroundTrafficThreadInfo* backgroundInfo){
    int to_send = backgroundInfo->requestQueue.size();

    for (int j = 0; j < to_send; ++j) {
        client* newClient;

        newClient = backgroundInfo->requestQueue.front();
        backgroundInfo->requestQueue.pop();

        std::vector<int64_t> to_send {newClient->index, (int64_t) backgroundInfo->my_workers.size(),
                                      backgroundInfo->total_bytes};
        put_request_in_buffer(newClient, to_send);
    }
}

/*
 * Create the callback container for the thread handling background traffic.
 */
CallbackInfo* create_background_callback(ThreadInfo* threadInfo){
    auto backgroundThreadInfo = (BackgroundTrafficThreadInfo*) threadInfo;

    auto* callbackInfo = new BackgroundCallbackInfo();
    callbackInfo->backgroundInfo = backgroundThreadInfo;
    callbackInfo->threadInfo = backgroundThreadInfo;

    return callbackInfo;
}

void start_background_thread(ExperimentInfo* info, BackgroundTrafficThreadInfo* backgroundInfo,
                             std::vector<IpPort> background){
    initialize_workers(info, 0, 0, background, backgroundInfo, create_background_connection, create_background_callback);
    connect_all_workers(info, backgroundInfo);

    sending_loop_background(info, backgroundInfo);

    event_base_dispatch(backgroundInfo->evbase);
    event_base_free(backgroundInfo->evbase);
}


int main(int argc, char* argv[]) {
    if (argc < 5){
        perror("Wrong number of parameters: payload_size number_threads workers_file rate_config_file "
               "[optional: background_workers_file total_background_bytes]");
        exit(EXIT_FAILURE);
    }

    ExperimentInfo* info = initialize_basic_sender(argv);
    std::string rate_config_file = argv[4];

    // You already have all the data, you can compute the schedule.
    compute_schedule(info, rate_config_file);

    // If the user provided a config with workers for background traffic initialize and run
    // the experiment including background traffic
    if (argc == 7){
        std::string fake_workers = argv[5];
        int background_bytes = atoi(argv[6]);

        std::vector<IpPort> background_workers;
        info->get_background_workers(fake_workers, background_workers);

        auto* backgroundInfo = new BackgroundTrafficThreadInfo();
        backgroundInfo->total_bytes = background_bytes;

        run_sender_with_background(info, background_workers, backgroundInfo, start_thread_rate, run_background);
    } else {
        run_experiment(info, start_thread_rate);
    }
}