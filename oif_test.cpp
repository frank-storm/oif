/*
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

#include <opencv2/opencv.hpp>

#include "oif.h"

#define IMG_WIDTH 1280
#define IMG_HEIGHT 720


int main (void)
{

    cv::Mat img(IMG_HEIGHT, IMG_WIDTH, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    cv::Mat logo;
    cv::Mat logo_alpha;
    cv::Mat dst_img(IMG_HEIGHT, IMG_WIDTH, CV_8UC4, cv::Scalar(0, 0, 0, 0));
    char c;
    int ret;

    struct oif_header header;
    unsigned char coding_buffer[IMG_HEIGHT * IMG_WIDTH * 4 + 256];

    std::cout << "OIF Test" << std::endl;
    std::cout << "Place a logo on an overlay screen and then compress" << std::endl;
    std::cout << "the screen with OIF." << std::endl;

    // Initialize header
    oif_init_header (&header, IMG_WIDTH, IMG_HEIGHT);

    // Add a logo as overlay
    logo = cv::imread ("logo.png", cv::IMREAD_UNCHANGED);

    cv::cvtColor (logo, logo_alpha, cv::COLOR_RGB2RGBA);
    cv::Mat roi(img, cv::Rect(100, 100, logo.cols, logo.rows));
    logo_alpha.copyTo(roi);

    // Compress the image
    oif_compress (&header, img.ptr<unsigned char>(0), coding_buffer);

    std::cout << "Uncompressed size: " << img.cols * img.rows * 4 << " bytes" << std::endl;
    std::cout << "Compressed size: " << header.img_size << " bytes" << std::endl;
    std::cout << "Compression ratio: " << (double) header.img_size /
        (double) (img.cols * img.rows * 4) << std::endl;

    // And uncompress it again
    ret = oif_uncompress (&header, coding_buffer, dst_img.ptr<unsigned char>(0));
    if (ret) {
        std::cout << "Error: Error while uncompressing image" << std::endl;
    }

    cv::imshow ("OIF Test", dst_img);

    while (1) {
        c = cv::waitKey(33);
        if (c == 'q') {
            break;
        }
    }
    return 0;
}


