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
int RS485_DELAY_AFTER = 0;

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
        if (tcsetattr(port_fd, TCSANOW, &old_tio) == -1) {
            perror("tcgetattr");
            exit(EXIT_FAILURE);
        }
        tcflush(port_fd, TCOFLUSH);
        tcflush(port_fd, TCIFLUSH);
        close(port_fd);
    }
    exit(EXIT_SUCCESS);
}

void print_help(char *argv0) {
    printf("Usage: %s [OPTION]... DEVICE\n", argv0);
    puts  ("io_hand -- communicate with serial port");
    puts  ("  -h            display this help and exit");
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
            exit(EXIT_SUCCESS);
        default:
            print_help(argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "expect device node after options\n");
        print_help(argv[0]);
        exit(EXIT_FAILURE);
    }

    char *dev = argv[optind];

    /* open serial port */
    port_fd = open(dev, O_RDWR | O_NOCTTY);

    if (port_fd < 0) {
        perror(dev);
        exit(EXIT_FAILURE);
    }

    /* register signal handler */
    if (tcgetattr(port_fd, &old_tio) == -1) {
        perror("io_hand tcgetattr");
        exit(EXIT_FAILURE);
    }

    memcpy(&tio, &old_tio, sizeof(struct termios));

    if (signal(SIGTERM, sig_handler) == SIG_ERR) {
        perror("register SIGTERM");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGINT, sig_handler) == SIG_ERR) {
        perror("register SIGINT");
        exit(EXIT_FAILURE);
    }

    /* config serial port */
    /* memcpy(&tio.c_cc, &old_tio.c_cc, NCCS); */
    tio.c_cflag = B9600 | CS8 | CREAD | CLOCAL;
    tio.c_iflag = IGNPAR | IGNBRK;
    tio.c_oflag = 0;
    tio.c_lflag = 0;
    tio.c_cc[VTIME] = 0;
    tio.c_cc[VMIN] = 1;
    if (tcsetattr(port_fd, TCSANOW, &tio) == -1) {
        perror("io_hand tcgetattr");
        exit(EXIT_FAILURE);
    }
    tcflush(port_fd, TCOFLUSH);
    tcflush(port_fd, TCIFLUSH);

    /* write and read */
    char buf[LINE_NR];
    char *tok;
    unsigned char idata[LINE_NR];
    int inr = 2, onr = 3, i, j, ret;
    char odata[3] = {0x8d};

    if (setvbuf(stdin, NULL, _IONBF, 0) != 0) {
        perror("setvbuf");
        exit(EXIT_FAILURE);    /* setvbuf error */
    }
    if (setvbuf(stdout, NULL, _IONBF, 0) != 0) {
        perror("setvbuf");
        exit(EXIT_FAILURE);    /* setvbuf error */
    }

    while (1) {
        fgets(buf, LINE_NR, stdin);
        tok = strtok(buf, " \n");
        if (strncmp(tok, "ping", 4) == 0) {
            fputs("pong", stdout);
        }
        else {
            odata[1] = atoi(tok) + 1;
            odata[2] = atoi(strtok(NULL, " \n")) + 1;
            if (write_rs485(port_fd, odata, onr) != onr)
                fprintf(stderr, "error: serial port %s I/O error\n", dev);
            read(port_fd, idata, inr);
        }
    }
}
