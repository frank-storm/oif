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
 *
 *
 *
 * oif = overlay image format
 *
 * The pixel data is 32 bit, 24 bit plus 8 bit alpha channel.
 * The image data is compressed with a run-length encoding (RLE).
 * In the compressed data, the data is split by 32 bit control
 * codes.
 * A code has the following format:
 *
 * Bit 31-28: Compression type
 * Bit 27-16: Line number
 * Bit 15-0:  Number of pixels
 *
 * If the compression type is RLE, the number of pixels defines
 * how often the following pixel value is repeated.
 * If the compression type is UNCOMPR, the number of pixels defines
 * the number of uncompressed pixels follwoing thes control code.
 * The WSL types (WSL = With Start Line) start at the specified line.
 * With the WSL types it is possible to send only partital stripes
 * of an overlay image.
 * The last code nust have the type EOI.
 *
 */

#ifndef OIF_H
#define OIF_H 1

#define OIF_MAGIC 0x4F494620  /* "OIF " */

/* The current version */
#define OIF_VERSION 1
#define OIF_SUBVERSION 0

#define OIF_UNCOMPR_TYPE 0x10000000
#define OIF_UNCOMPR_WSL_TYPE 0x20000000
#define OIF_RLE_TYPE 0x30000000
#define OIF_RLE_WSL_TYPE 0x40000000
#define OIF_EOI_TYPE 0xF0000000


#define OIF_ERR_UNKNWON_CODE -1
#define OIF_ERR_SRC_OVERRUN -2
#define OIF_ERR_DST_OVERRUN -3


struct oif_header {
    /* Format identifier, must be OIF_MAGIC */
    unsigned int magic;
    /* Verison number, split into two shorts */
    unsigned short version;
    unsigned short sub_version;
    /* Image size */
    unsigned int width;
    unsigned int height;
    /* The id field is implementation dependent. Typically it is
     * an identifier for an overlay, if multiple overlays are used and the
     * OIF files are transfered over the same channel. */
    int id;
    /* If != 0 the the image is uncompressed */
    int uncompressed;
    /* For future or user extensions */
    unsigned int reserved[8];
    /* Size of the image data following the image header */
    unsigned int img_size;
};


/*
 * Initializes an OIF header. The header can then be
 * directly used.
 */
extern void
oif_init_header (
    struct oif_header *header,
    unsigned int width,
    unsigned int height);

/*
 * Compresses an image data buffer.
 * The header must contain magic, width and height, and id.
 * The size is set after the compression,
 * compr_data must be a buffer with the same size as the input buffer.
 */
extern void
oif_compress (
    struct oif_header *header,
    unsigned char *img_data,
    unsigned char *compr_data);

/*
 * Uncompresses a compressed image.
 * The img_data must be a pointer to a memory area to contain the uncompressed
 * image.
 */
extern int
oif_uncompress (
    struct oif_header *header,
    unsigned char *compr_data,
    unsigned char *img_data);

#endif

