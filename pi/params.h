#ifndef PARAMS_H
#define PARAMS_H

typedef unsigned char byte;

//strip constants
#define STRIP_NUM_LEDS 330
#define STRIP_MAX_BRIGHTNESS 10

//serial
#define BAUD_RATE 1000000
#define DEV_PREAMBLE "/dev/ttyUSB"

//whoru
#define GPU_AID "GPU"
#define MIO_AID "MIO"
#define BTN0_AID "BTN0"
#define BTN1_AID "BTN1"

//CMD queue
#define CMD_PREAMBLE "CMD_"
#define CMD_WHORU '0'
#define CMD_DATA '1'
#define CMD_SYNC '2'
#define ENC_STREAM 0
#define ENC_RUN 1

//uncomment for single threaded
#define MULTITHREAD
//uncomment to use serial direct to buttons
#define NOMIDDLEMAN

#endif //PARAMS_H

