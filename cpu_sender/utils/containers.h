#ifndef SENDER_CONTAINERS_H
#define SENDER_CONTAINERS_H

#include <queue>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <unordered_set>
#include <netinet/in.h>
#include <fstream>

/*
 * IP-Port pair representing a worker.
 */
struct IpPort{
    int port;
    std::string ip;

    IpPort(int p, std::string& i){
        port = p;
        ip = i;
    }
};

/*
 * Information representing a worker: libevent variables, index of the worker, index of the thread the worker is
 * assigned to, and number of bytes already received.
 */
struct client {
    struct sockaddr_in server_address;
    struct bufferevent *buf_ev;
    struct evbuffer *output_buffer;
    int index;
    int previousBytes = 0;
    int threadIndex = 0;
    int expectedConnected = 0;

    struct event_base* evbase;
};

/*
 * Contains experiment information, common to all threads.
 */
struct ExperimentInfo{
    // List of workers
    std::vector<std::vector<IpPort>> workers;

    // Synchronization
    std::vector<int> notifyThreadsFds;
    std::mutex mutex;
    std::condition_variable condition;

    std::mutex sendMutex;

    // Experiment repetition
    std::vector<int> connectedCounter;
    int connectedThreads = 0;
    bool connectedBackground;

    // Parameters
    int CHUNK_SIZE, NUMBER_THREADS, ACTIVE_THREADS = 0, NUMBER_WORKERS;

    ExperimentInfo(int chunk_size, int number_threads){
        CHUNK_SIZE = chunk_size;
        NUMBER_THREADS = number_threads;

        notifyThreadsFds = std::vector<int>(NUMBER_THREADS, 0);
        connectedCounter = std::vector<int>(NUMBER_THREADS, 0);
        connectedBackground = false;
    }

    /*
     * Read the file containing the workers, registered as ip-port pairs
     */
    void read_file(std::string& config_file, std::vector<IpPort>& ip_port_pairs){
        std::ifstream infile(config_file);

        std::string line;
        while (std::getline(infile, line)) {
            std::size_t found = line.find(";");
            std::string host_ip = line.substr(0, found);
            int port = std::atoi(line.substr(found + 1).c_str());

            ip_port_pairs.emplace_back(IpPort(port, host_ip));
        }
    }

    /*
     * Parse the workers information and distribute them as equally as possible across the threads.
     */
    void get_workers(std::string& config_file){
        std::vector<IpPort> ip_port_pairs;

        read_file(config_file, ip_port_pairs);

        NUMBER_WORKERS = ip_port_pairs.size();

        int batchSize = floor((double) ip_port_pairs.size()/NUMBER_THREADS);

        std::chrono::high_resolution_clock::time_point start = std::chrono::high_resolution_clock::now();

        workers = std::vector<std::vector<IpPort>>(NUMBER_THREADS);

        for (int i = 0; i < NUMBER_WORKERS; ++i) {
            workers[i%NUMBER_THREADS].emplace_back(ip_port_pairs[i]);
        }
    }

    void get_background_workers(std::string& config_file, std::vector<IpPort>& background){
        read_file(config_file, background);
    }
};

/*
 * Contains all information related to one particular thread.
 */
class ThreadInfo {
public:
    std::unordered_map<int, std::chrono::high_resolution_clock::time_point> sendingTimes;
    std::unordered_map<int, std::chrono::high_resolution_clock::time_point> receivingTimes;
    std::unordered_map<int, std::chrono::high_resolution_clock::time_point> receivedACKTimes;
    std::unordered_map<int, std::chrono::high_resolution_clock::time_point> firstByteTimes;
    std::unordered_map<int, std::chrono::high_resolution_clock::time_point> startConnection;
    std::unordered_map<int, std::chrono::high_resolution_clock::time_point> finishConnection;
    std::unordered_map<int, client*> connections_information;

    std::queue<client*> requestQueue;
    std::unordered_set<int> received;
    std::unordered_set<int> my_workers;
    int expected;

    struct event_base* evbase;

    ThreadInfo(){}

    ThreadInfo(int expectedByThread){
        expected = expectedByThread;
    }
};

/*
 * Contains information required by threads that contact background traffic workers.
 */
class BackgroundTrafficThreadInfo: public ThreadInfo{
public:
    int total_bytes;
    int fake_workers_finished;
    int connectedCounter;

    BackgroundTrafficThreadInfo() : ThreadInfo(){
        connectedCounter = 0;
        fake_workers_finished = 0;
    }
};

/*
 * Contains information for all libevent callbacks.
 */
class CallbackInfo{
public:
    ExperimentInfo* info;
    client* client_info;
    ThreadInfo* threadInfo;
};

class BackgroundCallbackInfo : public CallbackInfo{
public:
    BackgroundTrafficThreadInfo* backgroundInfo;
};

/*
 * Contains information related to communication between threads.
 */
struct NotifyInfo{
    ExperimentInfo* info;
    ThreadInfo* threadInfo;
};


#endif
