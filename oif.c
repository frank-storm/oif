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


//#include <stdio.h>

#include "oif.h"


/*
 * Initializes an OIF header. The header can then be
 * directly used.
 */
void
oif_init_header (
    struct oif_header *header,
    unsigned int width,
    unsigned int height)
{
    int i;

    header->magic = OIF_MAGIC;
    header->version = OIF_VERSION;
    header->sub_version = OIF_SUBVERSION;
    header->width = width;
    header->height = height;
    header->id = 0;
    for (i = 0; i < 8; i++) {
        header->reserved[i] = 0;
    }
}


/*
 * Compresses an image data buffer.
 * The header must contain magic, width and height, and id.
 * The size is set after the compression,
 * compr_data must be a buffer with the same size as the input buffer.
 */
void
oif_compress (
    struct oif_header *header,
    unsigned char *img_data,
    unsigned char *compr_data)
{
    unsigned int i;
    unsigned int j;
    unsigned int k;
    unsigned int l;
    unsigned int code;
    unsigned int *curr_code = (unsigned int *) compr_data;
    unsigned int size;
    unsigned int *pixel_data = (unsigned int *) img_data;

    size = header->width * header->height;

    i = 0;
    j = 0;
    k = 0;
    while (i < size) {
        j = i + 1;
        while ((j < size) && (j - i < 32768) && (pixel_data[j] == pixel_data[i])) {
                j++;
            }
            if (j > i + 2) {
                /* Exceeds minimum number of equal pixels */
                if (k < i) {
                    /* There was uncompressed data before the sequence of equal pixels */
                    code = OIF_UNCOMPR_TYPE | (i - k);
                    *curr_code++ = code;
                    for (l = k; l < i; l++) {
                        *curr_code++ = pixel_data[l];
                    }
                }
                /* RLE for more than 3 repeated pixels */
                code = OIF_RLE_TYPE | (j - i);
                *curr_code++ = code;
                *curr_code++ = pixel_data[i];
                i = j;
                k = i;
            } else {
                i++;
            }
    }
    if (k < i) {
        /* There was uncompressed data before the sequence of equal pixels */
        code = OIF_UNCOMPR_TYPE | (i - k);
        *curr_code++ = code;
        for (l = k; l < i; l++) {
            *curr_code++ = pixel_data[i];
        }
    }
    code = OIF_EOI_TYPE;
    *curr_code++ = code;
    header->img_size = (unsigned int) ((unsigned char *) curr_code - compr_data);
}


/*
 * Uncompresses the compressed image data. img_data must be large enough
 * for the uncompresses image.
 */
int
oif_uncompress (
    struct oif_header *header,
    unsigned char *compr_data,
    unsigned char *img_data)
{
    unsigned int i;
    unsigned int code;
    unsigned int pixel_value = 0;
    unsigned int count;
    unsigned int line;
    unsigned int *curr_code = (unsigned int *) compr_data;
    unsigned int *curr_pixel = (unsigned int *) img_data;
    unsigned int *max_pixel = (unsigned int *) img_data + header->width * header->height;
    unsigned int *max_code = (unsigned int *) compr_data + header->img_size;

    code = *curr_code++;
    while ((code & 0xF0000000) != OIF_EOI_TYPE) {
        count = code & 0x0000FFFF;
        switch ((code & 0xF0000000)) {
        case OIF_UNCOMPR_TYPE:
            // printf ("OIF_UNCOMPR_TYPE, count = %d\n", count);
            if (curr_pixel + count > max_pixel) {
                return OIF_ERR_DST_OVERRUN;
            }
            if (curr_code + count > max_code) {
                return OIF_ERR_SRC_OVERRUN;
            }
            for (i = 0; i < count; i++) {
                *curr_pixel++ = *curr_code++;
            }
            break;
        case OIF_UNCOMPR_WSL_TYPE:
            line = (code >> 16) & 0x00000FFF;
            curr_pixel = (unsigned int *) (img_data +
                                            (line * header->width));
            if (curr_pixel + count > max_pixel) {
                return OIF_ERR_DST_OVERRUN;
            }
            if (curr_code + count > max_code) {
                return OIF_ERR_SRC_OVERRUN;
            }
            for (i = 0; i < count; i++) {
                *curr_pixel++ = *curr_code++;
            }
            break;
        case OIF_RLE_TYPE:
            // printf ("OIF_RLE_TYPE, count = %d\n", count);
            if (curr_pixel + count > max_pixel) {
                return OIF_ERR_DST_OVERRUN;
            }
            pixel_value = *curr_code++;
            if (curr_code > max_code) {
                return OIF_ERR_SRC_OVERRUN;
            }
            for (i = 0; i < count; i++) {
                *curr_pixel++ = pixel_value;
            }
            break;
        case OIF_RLE_WSL_TYPE:
            line = (code >> 16) & 0x00000FFF;
            curr_pixel = (unsigned int *) img_data +
                (line * header->width);
            if (curr_pixel + count > max_pixel) {
                return OIF_ERR_DST_OVERRUN;
            }
            pixel_value = *curr_code++;
            if (curr_code > max_code) {
                return OIF_ERR_SRC_OVERRUN;
            }
            for (i = 0; i < count; i++) {
                *curr_pixel++ = pixel_value;
            }
            break;
        default:
            return OIF_ERR_UNKNWON_CODE;
        }
        code = *curr_code++;
        if (curr_code > max_code) {
            return OIF_ERR_SRC_OVERRUN;
        }
    }
    return 0;
}



