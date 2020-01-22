#ifndef UTIL_H
#define UTIL_H

typedef unsigned long long now_t;
extern now_t ms_now_t;
extern now_t s_now_t;

void util_init();
now_t now();

#endif //UTIL_H

