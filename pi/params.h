#ifndef PARAMS_H
#define PARAMS_H

typedef unsigned char byte;
typedef byte nybl; //"should" be constrained to half-byte ("< 0x10"), but no easy way to do that

//strip constants
#define STRIP_NUM_LEDS 300

//serial
#define BAUD_RATE 1000000
#define DEV_PREAMBLE "/dev/ttyUSB"

//whoru
#define GPU_AID "GPU"
#define MIO_AID "MIO"
#define BTN_AID "BTN" //not used!

//CMD queue
#define CMD_PREAMBLE "CMD_"
#define CMD_WHORU '0'
#define CMD_DATA '1'

//uncomment for single threaded
#define MULTITHREAD

#endif //PARAMS_H

