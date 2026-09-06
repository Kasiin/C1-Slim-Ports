#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int emit(int fd, unsigned short type, unsigned short code, int value)
{
    struct input_event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.code = code;
    event.value = value;
    return write(fd, &event, sizeof(event)) == sizeof(event) ? 0 : -1;
}

int main(int argc, char **argv)
{
    if (argc != 3) {
        fprintf(stderr, "usage: evsend /dev/input/eventN key-code\n");
        return 2;
    }
    const int code = atoi(argv[2]);
    const int fd = open(argv[1], O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        fprintf(stderr, "open: %s\n", strerror(errno));
        return 1;
    }
    const int failed = emit(fd, EV_KEY, (unsigned short)code, 1) ||
                       emit(fd, EV_SYN, SYN_REPORT, 0) ||
                       emit(fd, EV_KEY, (unsigned short)code, 0) ||
                       emit(fd, EV_SYN, SYN_REPORT, 0);
    close(fd);
    if (failed) {
        fprintf(stderr, "write: %s\n", strerror(errno));
        return 1;
    }
    return 0;
}
