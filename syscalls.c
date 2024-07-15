// #include <sys/types.h>
// #include <sys/stat.h>
// #include <stdlib.h>
// #include <errno.h>
// #include <unistd.h>
// #include <time.h>

// #undef errno
// extern int errno;

// extern char _end; // Defined by the linker

// int _write(int file, char *ptr, int len) {
//     (void)file;
//     int i;
//     for (i = 0; i < len; i++) {
//         // Implementa la salida de caracteres, por ejemplo, a una UART
//         // Aquí usamos putchar para ejemplo
//         putchar(ptr[i]);
//     }
//     return len;
// }

// caddr_t _sbrk(int incr) {
//     static char *heap_end;
//     char *prev_heap_end;

//     if (heap_end == 0) {
//         heap_end = &_end;
//     }
//     prev_heap_end = heap_end;
//     heap_end += incr;

//     return (caddr_t)prev_heap_end;
// }

// int _close(int file) {
//     (void)file;
//     return -1;
// }

// int _fstat(int file, struct stat *st) {
//     (void)file;
//     st->st_mode = S_IFCHR;
//     return 0;
// }

// int _isatty(int file) {
//     (void)file;
//     return 1;
// }

// int _lseek(int file, int ptr, int dir) {
//     (void)file;
//     (void)ptr;
//     (void)dir;
//     return 0;
// }

// int _read(int file, char *ptr, int len) {
//     (void)file;
//     (void)ptr;
//     (void)len;
//     return 0;
// }

// void _exit(int status) {
//     (void)status;
//     while (1) {}
// }

// int _kill(int pid, int sig) {
//     (void)pid;
//     (void)sig;
//     errno = EINVAL;
//     return -1;
// }

// int _getpid(void) {
//     return 1;
// }

// // Define la estructura timezone
// struct timezone {
//     int tz_minuteswest; // Minutos al oeste de UTC
//     int tz_dsttime;     // Tipo de corrección de DST
// };

// int _gettimeofday(struct timeval *tv, struct timezone *tz) {
//     if (tv) {
//         tv->tv_sec = 0;
//         tv->tv_usec = 0;
//     }
//     if (tz) {
//         tz->tz_minuteswest = 0;
//         tz->tz_dsttime = 0;
//     }
//     return 0;
// }
