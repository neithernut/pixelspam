/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Julian Ganz
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
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

// library headers
#include <png.h>

// local headers
#include "util.h"




const size_t chunk_size = 1024*1024;
const unsigned int rounds = 100;




struct buf {
    struct buf* prev;
    size_t pos;
    char data[];
};


const size_t buflen = chunk_size - sizeof(struct buf);


struct buf* buf_new(struct buf* prev) {
    struct buf* retval = (struct buf*) malloc(chunk_size);
    retval->prev = prev;
    retval->pos = 0;
    return retval;
}


int buf_printf(struct buf** b, const char *format, ...) {
    va_list ap;

    struct buf* cur = *b;

    do {
        size_t remain = buflen - cur->pos;

        va_start(ap, format);
        int retval = vsnprintf(cur->data + cur->pos, remain, format, ap);
        va_end(ap);

        if (retval >= remain) {
            cur = buf_new(*b);
            if (!cur)
                return -1;
            *b = cur;
            continue;
        }

        if (retval > 0)
            cur->pos += retval;

        return retval;
    } while (1);
}


size_t buf_count(struct buf* b) {
    size_t retval = 0;
    while (b) {
        b = b->prev;
        ++retval;
    }
    return retval;
}




struct buf* encode_image(png_byte* data, int w, int h, int xoff, int yoff) {
    struct buf* b = buf_new(NULL);
    if (!b)
        die("Failed to alloc memory");

    const int n = w*h;
    for (unsigned int round = 0; round < rounds; ++round)
        for (int p = round; p < n; p += rounds) {
            const png_byte* const off = data + 4*p;
            const int x = p % w + xoff;
            const int y = p / w + yoff;

            if (off[3] < 128 || x < 0 || y < 0)
                continue;

            if (buf_printf(
                &b,
                "PX %d %d %02hhx%02hhx%02hhx\n",
                x,
                y,
                (unsigned char) off[0],
                (unsigned char) off[1],
                (unsigned char) off[2]
            ) < 0)
                return b;
        }

    return b;
}




struct {
    int x;
    int y;
} g = {.x = 0, .y = 0};




int main(int argc, char* argv[]) {
    switch(argc) {
    case 6:
        g.x = atoi(argv[3]);
        g.y = atoi(argv[4]);
    case 4:
        break;
    default:
        die("usage: pngspam host port file [x y]");
    };

    struct iovec* vec;
    int vec_len;
    {
        // read png
        FILE* file = fopen(argv[3], "rb");
        png_structp png = png_create_read_struct(
            PNG_LIBPNG_VER_STRING,
            NULL,
            NULL,
            NULL
        );
        if (!png)
            die("Failed to init libpng");
        png_infop info = png_create_info_struct(png);
        if (!info)
            die("Failed to init png info");

        if (setjmp(png_jmpbuf(png)))
            die("Failed to read png file");

        png_init_io(png, file);
        png_read_info(png, info);
        png_set_gray_to_rgb(png);
        png_set_expand(png);
        png_set_scale_16(png);
        if (png_get_color_type(png, info) != PNG_COLOR_TYPE_RGBA)
            die("Failed to read in RGBA format!");

        const int w = png_get_image_width(png, info);
        const int h = png_get_image_height(png, info);
        const size_t row_bytes = png_get_rowbytes(png, info);

        png_byte* data = (png_byte*) malloc(h*row_bytes);
        png_bytep* rows = (png_bytep*) malloc(h*sizeof(*rows));
        if (!data || !rows)
            die("Failed to alloc memory");
        for (size_t i = 0; i < h; ++i)
            rows[i] = data+i*row_bytes;

        png_read_image(png, rows);
        fclose(file);

        printf("Read PNG %s (%dx%d)\n", argv[3], w, h);

        struct buf* b = encode_image(data, w, h, g.x, g.y);
        vec_len = buf_count(b);
        if (!vec_len)
            die("No data to send!");
        vec = malloc(sizeof(*vec) * vec_len);
        size_t off = vec_len;
        while (off-- > 0) {
            vec[off].iov_base = b->data;
            vec[off].iov_len = b->pos;
            b = b->prev;
        }

        free(data);
        free(rows);
    }

    int sock = connect_to(argv[1], argv[2]);

    struct timespec ref_time;
    if (clock_gettime(CLOCK_MONOTONIC, &ref_time) < 0)
        die_errno("Failed to get some time ref");

    while (1) {
        // GREAT GLORY!!!!
        int res = writev(sock, vec, vec_len);
        if (res < 0)
            die_errno("Failed to push stuff");

        struct timespec curr_time;
        if (clock_gettime(CLOCK_MONOTONIC, &curr_time) < 0)
            die_errno("Failed to get some time ref");

        double dt = (curr_time.tv_sec  - ref_time.tv_sec ) +
                    (curr_time.tv_nsec - ref_time.tv_nsec) * 1e-9;
        printf("\r%6f imgs/s, %9ld kb/s", 1/dt, (long) (res/(dt*1000)));

        ref_time = curr_time;
    }
}

