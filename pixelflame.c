/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Julian Ganz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// sys headers
#include <sys/uio.h>

// std headers
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// library headers
#include <math.h>
#include <pthread.h>
#include <time.h>

// local headers
#include "util.h"




const size_t buflen = 1024*1024;
const unsigned short int iov_maxlen = 1024;
const double dt_target = 0.05;




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
    if (b->pos >= buflen)
        return 0;

    va_list ap;

    va_start(ap, format);
    int retval = vsnprintf(b->data + b->pos, buflen - b->pos, format, ap);
    va_end(ap);

    if (retval > 0)
        b->pos += retval;

    return retval;
}




struct guarded_vec {
    pthread_mutex_t lock;
    pthread_cond_t cond;
    struct iovec* data;
};


int guarded_vec_init(struct guarded_vec* vec) {
    int retval;

    retval = pthread_mutex_init(&vec->lock, NULL);
    if (retval < 0)
        return retval;
    retval = pthread_cond_init(&vec->cond, NULL);
    if (retval < 0)
        return retval;

    vec->data = NULL;
    return 0;
}


void guarded_vec_put(struct guarded_vec* vec, struct iovec* data) {
    pthread_mutex_lock(&vec->lock);

    while (vec->data)
        pthread_cond_wait(&vec->cond, &vec->lock);
    vec->data = data;
    pthread_cond_signal(&vec->cond);

    pthread_mutex_unlock(&vec->lock);
}


struct iovec* guarded_vec_get(struct guarded_vec* vec) {
    pthread_mutex_lock(&vec->lock);

    struct iovec* retval;
    while (!(retval = vec->data))
        pthread_cond_wait(&vec->cond, &vec->lock);

    pthread_mutex_unlock(&vec->lock);
    return retval;
}


void guarded_vec_release(struct guarded_vec* vec) {
    pthread_mutex_lock(&vec->lock);
    vec->data = NULL;
    pthread_cond_signal(&vec->cond);
    pthread_mutex_unlock(&vec->lock);
}




struct particle {
    unsigned int x;
    unsigned int y;
    unsigned char r;
    unsigned char g;
    unsigned char b;
};




struct {
    unsigned int x_max;
    unsigned int y_max;
    unsigned int particles;
} g = {.x_max = 2000, .y_max = 1000, .particles = 1024};




void prepare_job(
    struct buf* b,
    struct particle* ps,
    unsigned int x,
    unsigned int y,
    size_t new_particle,
    double hue
) {
    buf_reset(b);

    static const unsigned int x_variability = 2;
    static const unsigned int y_variability = 2;

    ps[new_particle].x = x;
    ps[new_particle].y = y;
    ps[new_particle].r = 192 + sin(hue + 0 * M_PI / 3) * 63;
    ps[new_particle].g = 192 + sin(hue + 2 * M_PI / 3) * 63;
    ps[new_particle].b = 192 + sin(hue + 4 * M_PI / 3) * 63;

    for (unsigned int i = 0; i < g.particles; ++i) {
        buf_printf(b, "PX %d %d %02hhx%02hhx%02hhx\n",
            ps[i].x,
            ps[i].y,
            ps[i].r,
            ps[i].b,
            ps[i].g
        );

        int r = rand();
        ps[i].x += r % (2 * x_variability + 1) - x_variability;
        r /= 2 * x_variability + 1;
        ps[i].y += r % (2 * y_variability + 1) - y_variability;
    }
}




void* do_work(void* vec) {
    struct guarded_vec* v = (struct guarded_vec*) vec;

    // configuration
    static const double colour_rate = 0.05;
    static const unsigned int rundown_speed = 1;

    // double buffers
    struct iovec* data[2];
    data[0] = malloc(sizeof(**data) * iov_maxlen);
    data[1] = malloc(sizeof(**data) * iov_maxlen);

    struct buf bufs[2];
    buf_init(bufs + 0);
    buf_init(bufs + 1);
    unsigned char buf_sel = 0;

    unsigned int x = rand() % g.x_max;
    unsigned int y = 0;

    struct particle* particles = malloc(sizeof(*particles) * g.particles);
    for (unsigned int i = 0; i < g.particles; ++i) {
        particles[i].x = x;
        particles[i].y = y;
        particles[i].r = 255;
        particles[i].g = 255;
        particles[i].b = 255;
    }
    size_t current_p = 0;
    double hue = 0;

    while (1) {
        buf_sel ^= 1;
        struct buf* buf = bufs + buf_sel;

        prepare_job(buf, particles, x, y, current_p, hue);

        if (current_p == 0)
            current_p = g.particles - 1;
        else
            --current_p;
        if ((hue += colour_rate) > (2 * M_PI))
            hue -= 2 * M_PI;
        if ((y += rundown_speed) > g.y_max) {
            y = 0;
            x = rand() % g.x_max;
        }

        struct iovec* d = data[buf_sel];
        for (size_t pos = 0; pos < iov_maxlen; ++pos) {
            d[pos].iov_base = buf->data;
            d[pos].iov_len = buf->pos;
        }
        guarded_vec_put(v, d);
    }
}




void* do_vacuum(void* sockptr) {
    int sock = *((int*) sockptr);

    void* data = malloc(buflen);
    while (read(sock, data, buflen) == 0);
}




int main(int argc, char* argv[]) {
    switch(argc) {
    case 6:
        g.particles = atoi(argv[5]);
    case 5:
        g.x_max = atoi(argv[3]);
        g.y_max = atoi(argv[4]);
    case 3:
        break;
    default:
        die("usage: pixelflame host port [x y [y_max {particles]]]");
    };

    int sock = connect_to(argv[1], argv[2]);

    // threads!!!
    pthread_t worker;
    pthread_t vacuumer;
    pthread_attr_t attr;
    if (pthread_attr_init(&attr) < 0)
        die_errno("Failed to init pthread attributes");

    struct guarded_vec vec;
    if (guarded_vec_init(&vec) < 0)
        die_errno("Failed to create exchange object\n");

    if (pthread_create(&worker, &attr, do_work, &vec) < 0)
        die_errno("Failed to create worker thread\n");

    if (pthread_create(&vacuumer, &attr, do_vacuum, &sock) < 0)
        die_errno("Failed to create vacuumer thread\n");

    unsigned short int vec_len = 1;

    struct timespec ref_time;
    if (clock_gettime(CLOCK_MONOTONIC, &ref_time) < 0)
        die_errno("Failed to get some time ref");

    while (1) {
        // GREAT GLORY!!!!
        struct iovec* v = guarded_vec_get(&vec);
        if (writev(sock, v, vec_len) < 0)
            die_errno("Failed to push stuff");
        guarded_vec_release(&vec);

        struct timespec curr_time;
        if (clock_gettime(CLOCK_MONOTONIC, &curr_time) < 0)
            die_errno("Failed to get some time ref");

        double dt = (curr_time.tv_sec  - ref_time.tv_sec ) +
                    (curr_time.tv_nsec - ref_time.tv_nsec) * 1e-9;

        const double bufs_per_sec = vec_len/(dt*1000);
        printf(
            "\r%6ld kbufs/s, %9ld kb/s",
            (long) bufs_per_sec,
            (long) (bufs_per_sec*v[0].iov_len)
        );

        vec_len = vec_len * (dt_target / dt);
        if (vec_len < 1)
            vec_len = 1;
        if (vec_len > iov_maxlen)
            vec_len = iov_maxlen;
        ref_time = curr_time;
    }
}

