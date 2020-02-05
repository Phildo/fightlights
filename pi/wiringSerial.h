#ifdef __cplusplus
extern "C" {
#endif

extern int   serialOpen     (const char *device, const int baud);
extern void  serialClose    (const int fd);
extern void  serialFlush    (const int fd);
extern int   serialPutchar  (const int fd, const unsigned char c);
extern int   serialPuts     (const int fd, const char *s);
extern int   serialPut      (const int fd, const char *s, const int n);
extern int   serialPrintf   (const int fd, const char *message, ...);
extern int   serialDataAvail(const int fd);
extern int   serialGetchar  (const int fd);

#ifdef __cplusplus
}
#endif

