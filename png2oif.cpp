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


int writeOifFile (
    std::string &fileName,
    struct oif_header *header,
    const char *imgData)
{
    std::ofstream wf (fileName, std::ios::out | std::ios::binary);

    wf.write((char *) header, sizeof (*header));
    wf.write(imgData, header->img_size);
    wf.close();
    return 0;
}


void setAlpha (
    cv::Mat &img,
    int bg_r,
    int bg_g,
    int bg_b)
{
    int channels = img.channels ();

    for (int y = 0; y < img.size().height; y++) {
        uchar* ptr = img.ptr (y);
        for (int x = 0; x < img.size ().width; x++) {
            if ((ptr[x * channels] == bg_r) && (ptr[x * channels + 1] == bg_g) &&
                    (ptr[x * channels + 2] == bg_b)) {
                ptr[x * channels + 3] = 0;
            } else {
                ptr[x * channels + 3] = 255;
            }
        }
    }
}


void usage ()
{
    std::cout << "Usage: png2oif [-h] [--help] [--usage] \\" << std::endl;
    std::cout << "               [-bg <red>,<green>,<blue>] \\" << std::endl;
    std::cout << "               [--background <red>,<green>,<blue>] \\" << std::endl;
    std::cout << "               <PNG image file name>" << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "    -h" << std::endl;
    std::cout << "    --help" << std::endl;
    std::cout << "    --usage                            Display this text" << std::endl;
    std::cout << "    -bg <red>,<green>,<blue>" << std::endl;
    std::cout << "    --background <red>,<green>,<blue>  If a PNG image does not have an" << std::endl;
    std::cout << "                                       alpha channel, the specified color" << std::endl;
    std::cout << "                                       is used as background color" << std::endl;
    std::cout << "                                       (alpha value = 0)" << std::endl;
    std::cout << std::endl;
    std::cout << "Converts a PNG file into the OIF format. If the PNG file does not" << std::endl;
    std::cout << "have an alpha channel, a background color can be specified." << std::endl;
    std::cout << std::endl;
}


int main (
    int argc,
    char* argv[])
{
    cv::Mat srcImg;
    cv::Mat dstImg;
    struct oif_header header;
    int i;
    int bg_r = -1;
    int bg_g = -1;
    int bg_b = -1;
    std::string oifFileName;
    std::string pngFileName;

    if (argc < 2) {
        std::cout << "png2oif <PNG file name>" << std::endl;
        return 1;
    }
    i = 1;
    while (i < argc) {
        std::string s = argv[i];
        if ((s.compare ("--help") == 0) || (s.compare ("--usage") == 0) ||
                (s.compare ("-h") == 0)) {
            usage ();
        } else if ((s.compare ("-bg") == 0) || (s.compare ("--background") == 0)) {
            i++;
            std::string bg_s  = argv[i];
            std::stringstream bg_stream(bg_s);
            std::string value;
            if (std::getline (bg_stream, value, ',')) {
                std::size_t pos;
                bg_r = std::stoi(value, &pos);
                if (pos == 0) {
                    std::cout << "Error: Invalid background value for red" << std::endl;
                    return 1;
                }
            }
            if (std::getline (bg_stream, value, ',')) {
                std::size_t pos;
                bg_g = std::stoi(value, &pos);
                if (pos == 0) {
                    std::cout << "Error: Invalid background value for green" << std::endl;
                    return 1;
                }
            } else {
                std::cout << "Error: Argument for -bg/--background must have the form <red>,<green>,<blue>" << std::endl;
                return 1;
            }
            if (std::getline (bg_stream, value, ',')) {
                std::size_t pos;
                bg_b = std::stoi(value, &pos);
                if (pos == 0) {
                    std::cout << "Error: Invalid background value for blue" << std::endl;
                    return 1;
                }
            } else {
                std::cout << "Error: Argument for -bg/--background must have the form <red>,<green>,<blue>" << std::endl;
                return 1;
            }
        } else {
            pngFileName = argv[i];
        }
        i++;
    }

    if (pngFileName.length() == 0) {
        std::cout << "Error: No PNG file specified" << std::endl;
        return 1;
    }

    std::cout << "Reading file " << pngFileName << std::endl;
    srcImg = cv::imread (pngFileName, cv::IMREAD_UNCHANGED);

    std::cout << "File has " << srcImg.channels () << " channels" << std::endl;

    // Initialize header
    oif_init_header (&header, srcImg.cols, srcImg.rows);

    if ((bg_r == -1) && (srcImg.channels () == 4)) {
        srcImg.copyTo(dstImg);
        oif_compress (&header, srcImg.ptr<unsigned char>(0), dstImg.ptr<unsigned char>(0));
    } else {
        cv::Mat srcImgAlpha;
        // Convert to RGBA
        cv::cvtColor (srcImg, srcImgAlpha, cv::COLOR_RGB2RGBA);
        srcImgAlpha.copyTo(dstImg);

        // Change color to alpha value
        if (bg_r != -1) {
            setAlpha (srcImgAlpha, bg_r, bg_g, bg_b);
        }

        // Compress the image
        oif_compress (&header, srcImgAlpha.ptr<unsigned char>(0), dstImg.ptr<unsigned char>(0));
    }

    std::cout << "Uncompressed size: " << srcImg.cols * srcImg.rows * 4 << std::endl;
    std::cout << "Compressed size: " << header.img_size << std::endl;
    std::cout << "Compression ratio: " << (double) header.img_size /
        (double) (srcImg.cols * srcImg.rows * 4) << std::endl;

    oifFileName = pngFileName.substr(0,pngFileName.find_last_of('.')) + ".oif";
    writeOifFile (oifFileName, &header, dstImg.ptr<char>(0));

    return 0;
}


