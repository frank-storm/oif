/*
 * Example program that receives OIF images over a socket connection
 * and writes them to a Linux framebuffer device.
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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>

#include "oif.h"


#define FB_DEVICE "/dev/fb0"

#define PORT 5018


void
oifServerLoop (
    int listenfd,
    unsigned char *frameBuffer,
    int fdFb)
{
    int size;
    int bufferSize;
    int sizeReceived;
    int connfd;
    unsigned char *rcvBuffer;
    unsigned char *currBufferPos;
    struct oif_header header;
    struct fb_var_screeninfo vinfo;
    int ret;

    // Get the screen info, the structure is later used to switch the frame halves
    ret = ioctl (fdFb, FBIOGET_VSCREENINFO, &vinfo);
    if (ret < 0) {
        printf ("Error: Cannot get framebuffer screen info (%s).\n", strerror (errno));
    }

    // Allocate the receive buffer
    bufferSize = vinfo.xres * vinfo.yres * sizeof (unsigned int);
    rcvBuffer = (unsigned char *) malloc (bufferSize);
    if (rcvBuffer == NULL) {
        printf ("Error: Cannot allocate memory.\n");
        return;
    }

    while (1) {
        connfd = accept (listenfd, (struct sockaddr*) NULL, NULL);
        if (connfd >= 0) {
            printf ("Connected.\n");

            while (1) {
                /* The connection is open until it is closed by a disconnect request
                 * or if we loose connection.
                 */
                size = read (connfd, &header, sizeof (header));
                if (size < 0) {
                    printf ("Error: %s\n", strerror (errno));
                    break;

                } else if (size == sizeof (header)) {
                    if (header.magic == OIF_MAGIC) {
                        // Some sanity checking
                        size = header.img_size;
                        if (size > bufferSize) {
                            break;
                        }
                        if ((header.width != vinfo.xres) || (header.width != vinfo.yres)) {
                            break;
                        }

                        // Receive the data
                        currBufferPos = rcvBuffer;
                        while (size > 0)  {
                            sizeReceived = read (connfd, currBufferPos, size);
                            if (size < 0) {
                                printf ("Error: %s\n", strerror (errno));
                            } else {
                                size -= sizeReceived;
                                currBufferPos += sizeReceived;
                            }
                        }
                        if (size < 0) {
                            break;
                        }

                        if (vinfo.yres_virtual > vinfo.yres) {
                            /* Use double-buffering, toggle between upper and lower frame buffer */
                            if (vinfo.yoffset > 0) {
                                vinfo.yoffset = 0;
                            } else {
                                vinfo.yoffset = vinfo.yres;
                            }
                            oif_uncompress (&header, rcvBuffer, frameBuffer +
                                            vinfo.yoffset * vinfo.xres * (vinfo.bits_per_pixel >> 3));

                            /* Now switch to the other half of the frame */
                            ret = ioctl (fdFb, FBIOPAN_DISPLAY, &vinfo);
                            if (ret < 0) {
                                printf ("Error: %s\n", strerror (errno));
                            }
                        } else {
                            oif_uncompress (&header, rcvBuffer, frameBuffer);
                        }
                    }
                } else {
                    printf ("Disconnected.\n");
                    break;
                }
            }
        }
    }
}


int
main (void)
{
    int listenfd = 0;
    struct sockaddr_in serv_addr;
    int fdFb;
    unsigned char *frameBuffer;
    struct fb_var_screeninfo vinfo;
    unsigned int size;
    int ret;

    printf ("OIF Example Server\n");

    fdFb = open (FB_DEVICE, O_RDWR);
    if (fdFb < 0) {
        printf ("Error: Cannot open framebuffer device \"%s\" (%s)\n",
                FB_DEVICE, strerror (errno));
        return -1;
    }
    ret = ioctl (fdFb, FBIOGET_VSCREENINFO, &vinfo);
    if (ret < 0) {
        printf ("Error: Cannot get framebuffer screen info (%s).\n", strerror (errno));
    }

    /* Map the frame buffer to user space */
    size = vinfo.xres * vinfo.yres * sizeof (unsigned int);
    frameBuffer = (unsigned char *) mmap (0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fdFb, 0);
    if (frameBuffer == MAP_FAILED) {
        close (fdFb);
        printf ("Error: Cannot map memory for framebuffer device %s\n", FB_DEVICE);
        return -1;
    }

    /* Open a socket connection */
    listenfd = socket (AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);

    memset (&serv_addr, '0', sizeof (serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl (INADDR_ANY);
    serv_addr.sin_port = htons (PORT);

    bind (listenfd, (struct sockaddr*) &serv_addr, sizeof (serv_addr));

    if (listen (listenfd, 10) == -1) {
        printf ("Error: Failed to listen\n");
        return -1;
    }

    oifServerLoop (listenfd, frameBuffer, fdFb);

    return 0;
}


