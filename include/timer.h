#ifndef _TIMER_H
#define _TIMER_H 

#include "common.h"

#define HZ 100

void init_timer();
void busy_wait(int millisecs);

typedef struct timeout_s timeout_t;
timeout_t* add_timeout(int millisecs);

#endif
