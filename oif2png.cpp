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
#include <fstream>

#include <opencv2/opencv.hpp>

#include "oif.h"


int readOifFile (
    std::string &fileName,
    struct oif_header *header,
    unsigned char **imgData)
{
    std::ifstream rf (fileName, std::ios::in | std::ios::binary);

    rf.read((char *) header, sizeof (*header));
    if (header->magic != OIF_MAGIC) {
        std::cout << "Error: Not a valid OIF file" << std::endl;
        rf.close();
        return -1;
    }

    *imgData = new unsigned char[header->img_size];
    if (! *imgData) {
        std::cout << "Error: Cannot allocate memory" << std::endl;
        rf.close();
        return -1;
    }

    rf.read((char *) *imgData, header->img_size);
    rf.close();
    return 0;
}


int main (
    int argc,
    char* argv[])
{
    struct oif_header header;
    unsigned char *data;
    std::string oifFileName;
    std::string pngFileName;
    int ret;

    if (argc < 2) {
        std::cout << "oif2png <OIF file name>" << std::endl;
        return 1;
    }

    oifFileName = argv[1];
    pngFileName = oifFileName.substr (0, oifFileName.find_last_of ('.')) + ".png";

    if (readOifFile (oifFileName, &header, &data)) {
        return 1;
    }

    cv::Mat img (header.height, header.width, CV_8UC4);

    ret = oif_uncompress (&header, data, (unsigned char *) img.ptr<char>(0));
    if (ret) {
        std::cout << "Error: Error while uncompressing image" << std::endl;
        return 1;
    }

    cv::imwrite (pngFileName, img);

    delete[] data;
    return 0;
}

