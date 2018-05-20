// sys headers
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

// std headers
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>




const size_t buflen = 1024*1024;
const unsigned short int iov_maxlen = 1024;


const struct addrinfo addr_hints = {
    .ai_flags = 0,
    .ai_family = AF_UNSPEC,
    .ai_socktype = SOCK_STREAM,
    .ai_protocol = 0,
    0
};




struct buf {
    char* data;
    size_t pos;
};


void buf_init(struct buf* b) {
    b->data = malloc(buflen);
    b->pos = 0;
}

void buf_reset(struct buf* b) {
    b->pos = 0;
}

int buf_printf(struct buf* b, const char *format, ...) {
    va_list ap;

    va_start(ap, format);
    int retval = vsnprintf(b->data + b->pos, buflen - b->pos, format, ap);
    va_end(ap);

    if (retval > 0)
        b->pos += retval;

    return retval;
}




struct {
    unsigned int x;
    unsigned int y;
    unsigned int w;
    unsigned int h;
} g = {.x = 0, .y = 0, .w = 100, .h = 100};




struct prepare_job {
    struct buf* buf;
    unsigned long int frame;
};


void* prepare_job_func(void* job) {
    // prepare the iov
    struct iovec* retval = malloc(sizeof(*retval) * iov_maxlen);
    {
        struct buf* b = ((struct prepare_job*) job)->buf;
        for (size_t pos = 0; pos < iov_maxlen; ++pos) {
            retval[pos].iov_base = b->data;
            retval[pos].iov_len = b->pos;
        }
    }

    unsigned long int frame = ((struct prepare_job*) job)->frame;

    // TODO: generate!

    return retval;
}




void die(const char* msg) {
        fprintf(stderr, "%s\n", msg);
        exit(1);
}


void die_errno(const char* msg) {
        fprintf(stderr, "%s: %s\n", msg, strerror(errno));
        exit(1);
}




int main(int argc, char* argv[]) {
    switch(argc) {
    case 6:
        g.h = atoi(argv[6]);
    case 5:
        g.w = atoi(argv[5]);
    case 4:
        g.y = atoi(argv[4]);
    case 3:
        g.x = atoi(argv[3]);
        break;
    default:
        die("usage: pixelspam host port [x [y [w [h]]]]");
    };

    // connect
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) die_errno("No sock!");
    {
        struct addrinfo* addresses;
        if (getaddrinfo(argv[1], argv[2], &addr_hints, &addresses) < 0)
            die_errno("Could not resolve address");

        // probe all addresses
        struct addrinfo* curr = addresses;
        while (curr) {
            if (connect(sock, curr->ai_addr, curr->ai_addrlen) >= 0)
                break;
            else
                fprintf(stderr, "Could not connect: %s", strerror(errno));
            curr = curr->ai_next;
        }
        freeaddrinfo(addresses);
        if (!curr) die("Could not connect!");
        fputs("Got a connection!\n", stderr);
    }

    // double buffer
    struct buf bufs[2];
    unsigned char buf_sel = 0;
    buf_init(bufs + 0);
    buf_init(bufs + 1);

    // threads!!!
    pthread_t worker;
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) < 0)
        die_errno("Failed to init pthread attributes");

    // misc initialization and allocation
    struct prepare_job job = {
        .buf = bufs,
        .frame = 0
    };
    struct iovec* vec = (struct iovec*) prepare_job_func(&job);
    unsigned short int vec_len = iov_maxlen;

    while (1) {
        // start a job, which will hopefully be completed before we finished writing
        buf_sel ^= 1;
        job.buf = bufs + buf_sel;
        ++job.frame;
        if (pthread_create(&worker, &attr, prepare_job_func, &job) < 0)
            die_errno("Failed to create worker thread\n");

        // GREAT GLORY!!!!
        if (writev(sock, vec, vec_len) < 0)
            die_errno("Failed to push stuff");
        free(vec);

        if (pthread_join(worker, (void**) &vec) < 0)
            die_errno("Failed to join, wtf?");
    }
}

