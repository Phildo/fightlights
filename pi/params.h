#ifndef PARAMS_H
#define PARAMS_H

typedef unsigned char byte;
typedef byte nybl; //"should" be constrained to half-byte ("< 0x10"), but no easy way to do that

//strip constants
#define STRIP_NUM_LEDS 300

//gpu arduino
#define GPU_FLUSH_TRIGGER "FLUSH"
#define GPU_BAUD_RATE 1000000
#define GPU_SERIAL_FILE "/dev/ttyUSB0"

//io arduino
#define IO_FLUSH_TRIGGER "FLUSH"
#define IO_BAUD_RATE 1000000
#define IO_SERIAL_FILE "/dev/ttyUSB1"

//CMD queue
#define CMD_PREAMBLE "CMD_"
#define CMD_WHORU '0'
#define CMD_DATA '1'

//uncomment for single threaded
#define MULTITHREAD

#endif //PARAMS_H

