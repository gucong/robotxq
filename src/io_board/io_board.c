#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#define LINE_NR 128
#define DATA_NR 96
#define READ_NR 12

/* RS485 delays (in millisecond) */
int RS485_DELAY_BEFORE = 0;
int RS485_DELAY_AFTER = 50;

int port_fd = -1;
struct termios old_tio;

/* send through rs485
 * return:    bytes send
 */
int write_rs485 (int fd, const char *data, const int nr)
{
    int size = 0;
    int state;

    if (nr == 0 || fd < 0)
        return 0;

    usleep(RS485_DELAY_BEFORE * 1000);

    /* send */
    size = write(fd, data, nr);

    /* wait for all data send */
    tcdrain(fd);
    usleep(RS485_DELAY_AFTER * 1000);

    return size;
}

/* sigterm handler to restore port */
static void sig_handler(int signo)
{
    if (port_fd != -1) {
        tcsetattr(port_fd, TCSANOW, &old_tio);
        tcflush(port_fd, TCOFLUSH);
        tcflush(port_fd, TCIFLUSH);
        close(port_fd);
    }
    exit(EXIT_SUCCESS);
}

void print_help(char *argv0) {
    printf("Usage: %s [OPTION]... DEVICE\n", argv0);
    puts  ("io_board -- read from rs485 serial port");
    puts  ("  -h            display this help and exit");
    puts  ("  -t TIME       delay TIME (ms) after request. (default: 50 ms)");
}

int main (int argc, char **argv)
{
    struct termios tio;

    int opt;
    while ((opt = getopt(argc, argv, "ht:")) != -1) {
        switch (opt) {
        case 't':
            RS485_DELAY_AFTER = atoi(optarg);
            break;
        case 'h':
            print_help(argv[0]);
            return 0;
        default:
            print_help(argv[0]);
            return 1;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "expect device node after options\n");
        print_help(argv[0]);
        return 1;
    }

    char *dev = argv[optind];

    /* open serial port */
    port_fd = open(dev, O_RDWR | O_NOCTTY);

    if (port_fd < 0) {
        perror(dev);
        return 1;
    }

    /* register signal handler */
    tcgetattr(port_fd, &old_tio);
    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
        perror("register SIGTERM");
        return 1;
    }

    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        perror("register SIGINT");
        return 1;
    }

    /* config serial port */
    /* memcpy(&tio.c_cc, &old_tio.c_cc, NCCS); */
    memcpy(&tio, &old_tio, sizeof(struct termios));
    tio.c_cflag = B9600 | CS8 | CREAD | CLOCAL;
    tio.c_iflag = IGNPAR | IGNBRK;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VTIME] = 0;
    tio.c_cc[VMIN] = 1;
    tcsetattr(port_fd, TCSANOW, &tio);
    tcflush(port_fd, TCOFLUSH);
    tcflush(port_fd, TCIFLUSH);

    /* write and read */
    unsigned char idata[LINE_NR];
    int inr = 35, i, j;
    char odata[3] = {0x8c, 0xab};

    int32_t data_save[DATA_NR];
    memset(data_save, 0, DATA_NR * sizeof(int));

    for (j = 1; j <= DATA_NR / 8; ++j) {
        odata[2] = j;
        if (write_rs485(port_fd, odata, 3) != 3)
            write(STDERR_FILENO, "not 3\n", 6);
        read(port_fd, idata, inr);

        //printf("%.2x: ", idata[2]);
        for (i = 3; i < inr; i+=4) {
            //printf("%15d ", *(int *)(idata+i));
            data_save[(j-1) * 8 + (i-3) / 4] = *(int32_t *)(idata+i);
        }
        //printf("\n");
    }

    char *bytes = (char *)data_save;
    for (i = 0; i < DATA_NR * 4; ++i) {
        putchar(bytes[i]);
    }

    raise(SIGTERM);
}
