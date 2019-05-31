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
#include "util.h"

// sys headers
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

// std headers
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static const struct addrinfo addr_hints = {
    .ai_flags = AI_CANONNAME,
    .ai_family = AF_UNSPEC,
    .ai_socktype = SOCK_STREAM,
    .ai_protocol = 0,
    0
};




void die(const char* msg) {
        fprintf(stderr, "%s\n", msg);
        exit(1);
}


void die_errno(const char* msg) {
        fprintf(stderr, "%s: %s\n", msg, strerror(errno));
        exit(1);
}


int connect_to(const char* host, const char* port) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) die_errno("No sock!");
    {
        struct addrinfo* addresses;
        if (getaddrinfo(host, port, &addr_hints, &addresses) < 0)
            die_errno("Could not resolve address");

        // probe all addresses
        struct addrinfo* curr = addresses;
        while (curr) {
            fprintf(stderr, "Connecting to %s...\n", curr->ai_canonname);
            if (connect(sock, curr->ai_addr, curr->ai_addrlen) >= 0)
                break;
            else
                fprintf(stderr, "Could not connect: %s\n", strerror(errno));
            curr = curr->ai_next;
        }
        freeaddrinfo(addresses);
        if (!curr) die("Could not connect!");
        fputs("Got a connection!\n", stderr);
    }
    return sock;
}

