#ifndef CPUSENDER_BASIC_SENDER_H
#define CPUSENDER_BASIC_SENDER_H

#include <queue>
#include <vector>
#include <math.h>
#include <event2/thread.h>
#include <event.h>
#include "utils/ThreadPool.h"
#include "utils/containers.h"

/*
 * Does initialization common to all types of senders.
 */
ExperimentInfo* initialize_basic_sender(char **argv);

/*
 * Save a timestamp once a request sent to a worker has been acked.
 */
void register_write(CallbackInfo* info);

/*
 * Runs the sender, including background traffic, waits for the experiment to finish and prints the results.
 */
void run_sender_with_background(ExperimentInfo *info, std::vector<IpPort> &backgroundWorkers,
        BackgroundTrafficThreadInfo* background, void(*thread_fun)(ExperimentInfo *, int, int, int, ThreadInfo *),
        void(*run_background)(ExperimentInfo *, ThreadPool *, BackgroundTrafficThreadInfo *, std::vector<IpPort>));

/*
 * Mockup function representing no background traffic ran.
 */
void no_background(ExperimentInfo*, ThreadPool*, BackgroundTrafficThreadInfo*, std::vector<IpPort>);

/*
 * Creates the callback container for all normal workers
 */
CallbackInfo* create_standard_callback(ThreadInfo* threadInfo);

/*
 * Runs the sender without background traffic.
 */
void run_experiment(ExperimentInfo* info, void(*thread_fun)(ExperimentInfo*,int,int,int,ThreadInfo*));

/*
 * Per thread: initializes all the information required to establish the connections to the workers.
 */
void initialize_workers(ExperimentInfo* info, int startIndex, int threadIndex, std::vector<IpPort>& workers,
        ThreadInfo* threadInfo, void (*create_connection)(CallbackInfo*), CallbackInfo*(create_callback)(ThreadInfo*));

/*
 * Starts the libevent loop and waits until all the worker connections are ready.
 */
void connect_all_workers(ExperimentInfo* info, ThreadInfo* threadInfo);

/*
 * Run one sender thread.
 */
void start_thread(ExperimentInfo* info, int startIndex, int threadIndex, int notifyPipe, ThreadInfo* threadInfo,
        void (*sending_loop)(ExperimentInfo*,ThreadInfo*),
        void (*create_connection)(CallbackInfo*),
        void (*available_token_notification)(evutil_socket_t,short,void*));

/*
 * Standard libevent callback: once a connection has been established, register the timestamp. Exit the loop once all
 * connections are ready.
 */
void on_connection(struct bufferevent *bev, short what, void *arg);

/*
 * Initialize all libevent variables required for a single worker.
 */
void create_connection(ThreadInfo* threadInfo, client* client_info);

/*
 * Process a reply from a particular worker.
 */
void handle_reply(struct bufferevent *bev, CallbackInfo* info, void (*reply_preprocess)(CallbackInfo*),
                  void (*reply_postprocess)(CallbackInfo* info));

/*
 * After the sender has finished, print all the event timestamps.
 */
void print_results(std::vector<ThreadInfo*> list, std::chrono::high_resolution_clock::time_point start);

/*
 * Puts all the information to send as request in the output buffer of a particular worker.
 */
void put_request_in_buffer(client* client_info, std::vector<int64_t>& to_send);

/*
 * Sends a wake up message to all threads.
 */
void notify_all_threads(ExperimentInfo* info);

/*
 * Starts the process of sending a request to a worker. It saves the timestamp indicating when the process started.
 */
void request_to_send(ThreadInfo* threadInfo, client* client_info, std::vector<int64_t>& to_send);

#endif
