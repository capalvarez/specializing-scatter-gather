#include <arpa/inet.h>
#include <unordered_set>
#include "basic_sender.h"

ExperimentInfo* initialize_basic_sender(char **argv){
    int CHUNK_SIZE = atoi(argv[1]);
    int NUMBER_THREADS = atoi(argv[2]);
    std::string workers_file = argv[3];

    cpu_set_t my_set;
    CPU_ZERO(&my_set);
    CPU_SET(NUMBER_THREADS+1, &my_set);
    sched_setaffinity(0, sizeof(cpu_set_t), &my_set);

    auto* info = new ExperimentInfo(CHUNK_SIZE, NUMBER_THREADS);
    info->get_workers(workers_file);

    return info;
}

void register_write(CallbackInfo* info){
    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    info->threadInfo->receivedACKTimes[info->client_info->index] = now;
}

void run_sender_with_background(ExperimentInfo *info, std::vector<IpPort> &backgroundWorkers,
        BackgroundTrafficThreadInfo* background, void(*thread_fun)(ExperimentInfo *, int, int, int, ThreadInfo *),
        void(*run_background)(ExperimentInfo *, ThreadPool *, BackgroundTrafficThreadInfo *, std::vector<IpPort>)){
    ThreadPool* pool = new ThreadPool(info->NUMBER_THREADS);
    pool->set_affinity();

    evthread_use_pthreads();

    std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();
    std::vector<ThreadInfo*> threadInfo_list;

    for (int j = 0; j < info->NUMBER_THREADS; ++j) {
        std::vector<IpPort> worker = info->workers[j];

        if(worker.empty()){
            continue;
        }

        int fds[2];
        if(pipe(fds)){}

        info->notifyThreadsFds[j] = fds[1];

        info->ACTIVE_THREADS++;

        ThreadInfo* threadInfo = new ThreadInfo(info->workers[j].size());
        threadInfo->expected = worker.size();

        threadInfo_list.emplace_back(threadInfo);

        pool->enqueue([thread_fun, info, j, fds, threadInfo]{
            return thread_fun(info, j, j, fds[0], threadInfo);});
    }

    run_background(info, pool,background, backgroundWorkers);
    pool->wait_for_finish();

    print_results(threadInfo_list, start);
}

void no_background(ExperimentInfo*, ThreadPool*, BackgroundTrafficThreadInfo*, std::vector<IpPort>){}

CallbackInfo* create_standard_callback(ThreadInfo* threadInfo){
    CallbackInfo* callbackInfo = new CallbackInfo();
    callbackInfo->threadInfo = threadInfo;

    return callbackInfo;
}

void run_experiment(ExperimentInfo* info,
        void(*thread_fun)(ExperimentInfo*,int,int,int,ThreadInfo*)) {
    std::vector<IpPort> background;
    BackgroundTrafficThreadInfo* backgroundTrafficThreadInfo = new BackgroundTrafficThreadInfo();

    run_sender_with_background(info, background, backgroundTrafficThreadInfo, thread_fun, no_background);
}

void initialize_workers(ExperimentInfo* info, int startIndex, int threadIndex, std::vector<IpPort>& workers,
        ThreadInfo* threadInfo, void (*create_connection)(CallbackInfo*), CallbackInfo*(create_callback)(ThreadInfo*)){
    struct event_base* evbase;
    struct event_config *cfg;

    cfg = event_config_new();
    event_config_set_flag(cfg, EVENT_BASE_FLAG_NO_CACHE_TIME);
    event_config_set_flag(cfg, EVENT_BASE_FLAG_PRECISE_TIMER);

    if ((evbase = event_base_new_with_config(cfg)) == NULL) {
        perror("client event_base creation failed");
        return;
    }

    threadInfo->evbase = evbase;

    std::queue<client*> myClients;

    for (int i = 0; i < workers.size(); ++i) {
        int port = workers[i].port;
        std::string server_ip = workers[i].ip;

        auto client_info = new client();
        client_info->index = startIndex + info->ACTIVE_THREADS * i;
        client_info->threadIndex = threadIndex;
        client_info->expectedConnected = workers.size();
        client_info->evbase = evbase;

        threadInfo->my_workers.insert(startIndex + info->ACTIVE_THREADS * i);

        if ((client_info->output_buffer = evbuffer_new()) == NULL) {
            perror("client output buffer allocation failed");
            return;
        }

        client_info->server_address.sin_family = AF_INET;
        client_info->server_address.sin_port = htons(port);

        if(inet_pton(AF_INET, server_ip.c_str(), &client_info->server_address.sin_addr) <= 0){
            perror("Invalid server IP address");
            exit(EXIT_FAILURE);
        }

        auto callbackInfo = create_callback(threadInfo);

        callbackInfo->info = info;
        callbackInfo->client_info = client_info;

        create_connection(callbackInfo);
        myClients.emplace(client_info);
    }

    threadInfo->requestQueue = myClients;
}

void connect_all_workers(ExperimentInfo* info, ThreadInfo* threadInfo){
    event_base_priority_init(threadInfo->evbase, 3);
    event_base_dispatch(threadInfo->evbase);

    {
        std::unique_lock<std::mutex> lock(info->mutex);
        info->condition.wait(lock, [info]{ return info->ACTIVE_THREADS <= info->connectedThreads; });
    }
}


void start_thread(ExperimentInfo* info, int startIndex, int threadIndex, int notifyPipe, ThreadInfo* threadInfo,
        void (*sending_loop)(ExperimentInfo*,ThreadInfo*), void (*create_connection)(CallbackInfo*),
        void (*available_token_notification)(evutil_socket_t,short,void*)) {
    initialize_workers(info, startIndex, threadIndex, info->workers[threadIndex], threadInfo, create_connection,
            create_standard_callback);
    connect_all_workers(info, threadInfo);

    NotifyInfo* notifyInfo = new NotifyInfo();
    notifyInfo->info = info;
    notifyInfo->threadInfo = threadInfo;

    struct event* myNotifyEvent = event_new(threadInfo->evbase, notifyPipe, EV_READ|EV_PERSIST, available_token_notification, (NotifyInfo*)notifyInfo);
    event_add(myNotifyEvent, NULL);

    sending_loop(info, threadInfo);

    event_base_dispatch(threadInfo->evbase);
    event_base_free(threadInfo->evbase);
}


void handle_reply(struct bufferevent *bev, CallbackInfo* info, void (*reply_preprocess)(CallbackInfo*),
        void (*reply_postprocess)(CallbackInfo* info)) {
    char buffer[info->info->CHUNK_SIZE];
    int bytesRead = 0;
    int result;

    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

    while ((bytesRead = EVBUFFER_LENGTH(bev->input)) > 0 && info->client_info->previousBytes < info->info->CHUNK_SIZE) {
        if(info->client_info->previousBytes == 0){
            info->threadInfo->firstByteTimes.insert(std::make_pair(info->client_info->index, now));
        }

        if (bytesRead > 4096){
            bytesRead = 4096;
        }

        result = evbuffer_remove(bev->input, buffer + info->client_info->previousBytes, info->info->CHUNK_SIZE > bytesRead? info->info->CHUNK_SIZE - bytesRead : bytesRead);
        info->client_info->previousBytes += result;
    }

    if(info->client_info->previousBytes >= info->info->CHUNK_SIZE) {
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

void on_connection(struct bufferevent *bev, short what, void *arg) {
    CallbackInfo* callbackInfo = (CallbackInfo*) arg;
    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();

    {
        std::unique_lock<std::mutex> lock(callbackInfo->info->mutex);
        callbackInfo->info->connectedCounter[callbackInfo->client_info->threadIndex]++;

        callbackInfo->threadInfo->connections_information[callbackInfo->client_info->index] = callbackInfo->client_info;
        callbackInfo->threadInfo->finishConnection[callbackInfo->client_info->index] = now;

        if (callbackInfo->client_info->expectedConnected <= callbackInfo->info->connectedCounter[callbackInfo->client_info->threadIndex]){
            callbackInfo->info->connectedThreads++;

            callbackInfo->info->condition.notify_all();
            event_base_loopbreak(callbackInfo->client_info->evbase);
        }
    }
}

void create_connection(ThreadInfo* threadInfo, client* client_info){
    if ((client_info->buf_ev = bufferevent_socket_new(client_info->evbase, -1,
                                                      BEV_OPT_CLOSE_ON_FREE)) == NULL) {
        perror("client bufferevent creation failed");
        return;
    }

    threadInfo->startConnection[client_info->index] = std::chrono::high_resolution_clock::now();

    bufferevent_socket_connect(client_info->buf_ev, (sockaddr * ) &client_info->server_address,
                               sizeof(client_info->server_address));
    bufferevent_base_set(client_info->evbase, client_info->buf_ev);
    bufferevent_enable(client_info->buf_ev, EV_READ | EV_WRITE | EV_PERSIST);
}

void print_results(std::vector<ThreadInfo*> list, std::chrono::high_resolution_clock::time_point start){
    std::chrono::high_resolution_clock::time_point minSending = list[0]->sendingTimes[0];
    std::chrono::high_resolution_clock::time_point maxReceiving = list[0]->receivingTimes[0];

    for (int k = 0; k < list.size(); ++k) {
        ThreadInfo* info = list[k];

        for (auto sendTime: info->sendingTimes){
            auto startConnection = info->startConnection[sendTime.first];
            auto finishConnection = info->finishConnection[sendTime.first];
            auto receivedAck = info->receivedACKTimes[sendTime.first];
            auto firstByte = info->firstByteTimes[sendTime.first];
            auto receiveTime = info->receivingTimes[sendTime.first];

            std::cout << std::to_string(sendTime.first) << " "
                      << std::chrono::duration_cast<std::chrono::microseconds>(startConnection - start).count() << " "
                      << std::chrono::duration_cast<std::chrono::microseconds>(finishConnection - start).count() << " "
                      << std::chrono::duration_cast<std::chrono::microseconds>(sendTime.second - start).count() << " "
                      << std::chrono::duration_cast<std::chrono::microseconds>(receivedAck - start).count() << " "
                      << std::chrono::duration_cast<std::chrono::microseconds>(firstByte - start).count() << " "
                      << std::chrono::duration_cast<std::chrono::microseconds>(receiveTime - start).count()
                      << std::endl;

            if (sendTime.second < minSending){
                minSending = sendTime.second;
            }

            if (receiveTime > maxReceiving){
                maxReceiving = receiveTime;
            }
        }
    }

    std::cout << "Total time: "
              << std::chrono::duration_cast<std::chrono::microseconds>(maxReceiving - minSending).count()
              << std::endl;
}

void put_request_in_buffer(client *client_info, std::vector<int64_t>& to_send) {
    for (int64_t e: to_send){
        evbuffer_add(client_info->output_buffer, (char*)&e, sizeof(e));
    }

    client_info->previousBytes = 0;
    bufferevent_write_buffer(client_info->buf_ev, client_info->output_buffer);
}

void notify_all_threads(ExperimentInfo *info) {
    for (int i = 0; i < info->notifyThreadsFds.size(); ++i) {
        int notifyFd = info->notifyThreadsFds[i];

        if(notifyFd > 0){
            char buf[1];
            buf[0] = 'c';

            write(notifyFd, buf, 1);
        }
    }
}

void request_to_send(ThreadInfo* threadInfo, client* client_info, std::vector<int64_t>& to_send){
    std::chrono::high_resolution_clock::time_point now = std::chrono::high_resolution_clock::now();
    put_request_in_buffer(client_info, to_send);

    threadInfo->sendingTimes.insert(std::make_pair(client_info->index, now));
}

