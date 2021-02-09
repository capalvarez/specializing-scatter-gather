#ifndef BASE_WORKER_H
#define BASE_WORKER_H

#include "config/base_config.h"

/*
 * Receive data from a socket and parse it to get a 64-bit integer.
 */
int64_t receive_int(int64_t *num, int fd);

/*
 * Basic loop that all worker implementations use: it creates the connection socket, pins the process to a core, and
 * starts an endless loop accepting connections.
 */
void base_worker_loop(BaseConfig* baseConfig, void (*serve)(int,BaseConfig*));

#endif
