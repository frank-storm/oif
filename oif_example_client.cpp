/*
 * Example program that sends a moving logo as overlay to an
 * OIF server over a socket connection.
 *
 * Copyright (C) 2023 by Frank Storm <frank.storm@storm-se.com>
 *
 * Permission to use, copy, modify, and/or distribute this software
 * for any purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */


#include <iostream>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <opencv2/opencv.hpp>

#include "oif.h"

// Adjust to the actual display size
#define IMG_WIDTH 1600
#define IMG_HEIGHT 720

// Increment per move
#define DX 3
#define DY 4

// Curremt logo position, start at x=100, y=100
int logo_x = 100;
int logo_y = 100;

int dx = DX;
int dy = DY;


// The logo is moving around the screen. if it reaches a border
// it is bouncing back.
void
calculateLogoPosition (
    int logoWidth,
    int logoHeight)
{
    int x0;
    int x1;
    int y0;
    int y1;

    x0 = logo_x + dx;
    x1 = logo_x + logoWidth + dx;
    y0 = logo_y + dy;
    y1 = logo_y + logoHeight + dy;

    if (dy < 0) {
        if (y0 < 0) {
            dy = 0 - dy;
            y0 = logo_y;
        }
    } else {
        if (y1 > IMG_HEIGHT) {
            dy = 0 - dy;
            y0 = logo_y;
        }
    }
    if (dx < 0) {
        if (x0 < 0) {
            dx = 0 - dx;
            x0 = logo_x;
        }
    } else {
        if (x1 > IMG_WIDTH) {
            dx = 0 - dx;
            x0 = logo_x;
        }
    }

    logo_x = x0;
    logo_y = y0;
}


#define NANOSECONDS_PER_SECOND		(1000000000ULL)

void
waitForEndOfInterval (
    unsigned long interval,
    struct timespec *start)
{
    unsigned long long time_elapsed;
    unsigned long long long_interval;
    struct timespec now;
    struct timespec rem;
    clockid_t clkid = CLOCK_REALTIME;

    clock_gettime (clkid, &now);
    time_elapsed = (now.tv_sec - start->tv_sec)*NANOSECONDS_PER_SECOND + (now.tv_nsec - start->tv_nsec);
    long_interval = interval;
    if (time_elapsed < long_interval) {
        rem.tv_sec = (long_interval - time_elapsed) / NANOSECONDS_PER_SECOND;
        rem.tv_nsec = (long_interval - time_elapsed) % NANOSECONDS_PER_SECOND;
        nanosleep (&rem, NULL);
    }
}


void
usage (
    char *prog)
{
    std::cout << "usage: " << prog << " <ip-addr> [<port-number>]" << std::endl;
}


int
main (
    int argc,
    char *argv[])
{
    cv::Mat img(IMG_HEIGHT, IMG_WIDTH, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    cv::Mat logo;
    cv::Mat logo_alpha;
    int sockfd;
    struct sockaddr_in serv_addr;
    int size;
    char *ipAddr;
    struct timespec now;
    clockid_t clkid = CLOCK_REALTIME;

    // 30 fps
    unsigned long delay = 33333333;

    // 60 fps
    // unsigned long delay = 16666666;
    int ret;
    int port;
    char *endptr;


    struct oif_header header;
    unsigned char coding_buffer[IMG_HEIGHT * IMG_WIDTH * 4 + 256];

    // We need at least an IP address as argument
    if ((argc != 2) && (argc != 3)) {
        usage (argv[0]);
        return 1;
    }
    // Whether the ip address is valid is checked when we try to connect
    ipAddr = argv[1];

    if (argc == 3) {
        // We also have a port number
        port = strtoul (argv[2], &endptr, 10);
    } else {
        port = 5018;
    }

    std::cout << "OIF Example Client" << std::endl;

    // Open a socket connection
    if ((sockfd = socket (AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "Error: Could not create socket" << std::endl;
        return 1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons (port);
    serv_addr.sin_addr.s_addr = inet_addr (ipAddr);

    ret = connect (sockfd, (struct sockaddr *) &serv_addr, sizeof (serv_addr));
    if (ret < 0) {
        std::cout << "Error: Connect failed (" << strerror(errno) << ")" << std::endl;
        return 1;
    }

    // Initialize the OIF header
    header.magic = OIF_MAGIC;
    header.width = IMG_WIDTH;
    header.height = IMG_HEIGHT;
    header.id = 1;

    // Read the logo
    logo = cv::imread ("logo.png", 1);
    cv::cvtColor (logo, logo_alpha, cv::COLOR_RGB2RGBA);

    clock_gettime (clkid, &now);
    while (1) {
        // Clear image
        img = cv::Mat::zeros(img.size(), img.type());
        // and copy the logo to the new position
        cv::Mat roi(img, cv::Rect(logo_x, logo_y, logo.cols, logo.rows));
        logo_alpha.copyTo(roi);

        // Compress the image
        oif_compress (&header, img.ptr<unsigned char>(0), coding_buffer);

        // Report statistics
        std::cout << "Uncompressed size:" << img.cols * img.rows * 4 << std::endl;
        std::cout << "Compressed size:" << header.img_size << std::endl;
        std::cout << "Compression ratio:" << (double) header.img_size /
            (double) (img.cols * img.rows * 4) << std::endl;

        std::cout << "Sending image..." << std::endl;

        // Allign the sending of the overlay to 30 fps
        waitForEndOfInterval (delay, &now);
        clock_gettime (clkid, &now);

        // Send the overlay. First send the header
        size = write (sockfd, &header, sizeof (header));
        if (size < 0) {
            std::cout << "Error: Cannot send image header (" << strerror(errno) << ")" << std::endl;
            close (sockfd);
            return 1;
        }
        // and the the image data
        size = write (sockfd, coding_buffer, header.img_size);
        if (size < 0) {
            std::cout << "Error: Cannot send image data (" << strerror(errno) << ")" << std::endl;
            close (sockfd);
            return 1;
        }

        if (size != (int) header.img_size) {
            std::cout << "Error: Image data not fully sent" << std::endl;
            close (sockfd);
            return 1;
        }

        // Finally calculate a new position
        calculateLogoPosition (logo.cols, logo.rows);
    }
    close (sockfd);
    return 0;
}


