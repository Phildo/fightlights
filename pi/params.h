typedef char byte;
typedef byte nybl; //"should" be constrained to half-byte, but no easy way to do that

//strip constants
#define STRIP_NUM_LEDS 300

//sync w/ arduino
#define FLUSH_TRIGGER "FLUSH"
#define BAUD_RATE 1000000
#define SERIAL_FILE "/dev/ttyUSB0"

