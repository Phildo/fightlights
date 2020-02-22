#ifndef UTIL_H
#define UTIL_H

typedef unsigned char byte;

//color
typedef struct { byte r; byte g; byte b; } color;
color ncolor(byte r, byte g, byte b);
color dampen_color(color in, float amt);
color hdampen_color(color in, int amt);
color mix_color(color a, color b);
int color_cmp(color a, color b);

//timing
typedef unsigned long long now_t;
extern now_t ms_now_t;
extern now_t s_now_t;

void util_init();
now_t now();

#endif //UTIL_H

