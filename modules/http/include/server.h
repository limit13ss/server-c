#ifndef NAZARK_SERVER_H
#define NAZARK_SERVER_H

#define SERVER_PORT 1337
#define SERVER_REQUESTS_QUEUE_SIZE 128

#include "stdint.h"

int32_t server_initMainListener(void);
void server_mainLoop(void);

#endif // NAZARK_SERVER_H
