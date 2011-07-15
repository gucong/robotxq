#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
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
/* read pass */
int PASS_NR = 4;

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

int main (int argc, char ** argv)
{
    struct termios tio;

    if (argc < 2) {
        printf("usage: %s DEVICE [PASS] [DELAY(ms)]\n", argv[0]);
        return 1;
    }

    if (argc >= 3) {
        PASS_NR = atoi(argv[2]);
    }
    if (argc >= 4) {
        RS485_DELAY_AFTER = atoi(argv[3]);
    }

    /* open serial port */
    port_fd = open(argv[1], O_RDWR | O_NOCTTY);

    if (port_fd < 0) {
        perror(argv[1]);
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
    int inr = 35, i, j, pass;
    char odata[3] = {0x8c, 0xab};

    int data_save[DATA_NR];
    memset(data_save, 0, DATA_NR * sizeof(int));

    for (pass = 0; pass < PASS_NR; ++pass) {
        for (j = 1; j <= DATA_NR / 8; ++j) {
            odata[2] = j;
            if (write_rs485(port_fd, odata, 3) != 3)
                write(STDERR_FILENO, "not 3\n", 6);
            read(port_fd, idata, inr);

            //printf("%.2x: ", idata[2]);
            for (i = 3; i < inr; i+=4) {
                //printf("%15d ", *(int *)(idata+i));
                int index = (j-1) * 8 + (i-3) / 4;
                if (data_save[index] == 0)
                    data_save[index] = *(int *)(idata+i);
            }
            //printf("\n");
        }
    }

    char *bytes = (char *)data_save;
    for (i = 0; i < DATA_NR * 4; ++i) {
        putchar(bytes[i]);
    }

    raise(SIGTERM);
}
