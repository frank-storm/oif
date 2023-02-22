
#OIF - Overlay Image Format


The Overlay Image Format (OIF) was created because of the need for an
image format that could be used to send overlay data over a network.
The overlay in this case is not meant to be a video (picture in picture)
but instruments, logos, menu bars, etc.
The target is embedded devices, so it should only have little overhead
and should not be compute intense. Since the image format is for overlays,
an alpha channel is a must.
The following is a list of requirements for an overlay format:

- Must support compression. OIF supports run length encoding (RLE), which can
be implemented with a minimum computational overhead. Most overlays only
cover a part of the screen and often consist of regular structures, so RLE
is a good tradeoff between compression and computational overhead.
The actual implementation is such that it could be easily done completely in hardware (FPGA).
- Should support a variable frame rate. The overlay should only be updated
when needed. Sending the oberlay as a video stream is possible but causes
unnecessary traffic when sent over a network. As a consequence, the overlay
should be sent as TCP packets instead of a UDP stream.
- For most applications, 32 bit RGBA is sufficient. Therefore this is
also the only format supported by OIF.
- Most overlays consist of different sections (e.g. menu bar at the top, status
bar at the bottom). Therefore it would be a good idea to only send those parts of the
overlay that have actually changed. OIF supports this by transporting only one or multiple
regions of lines. How OIF is used depends on the use case and the choice of
the user. But this feature allows for creating OIF files by different applications
for different parts of the overlay.
- Modern systems do not only support one display. Therefore OIF contains on ID field
in its header to send different overlays over the same channel.


## Examples

The source code contains the actual OIF implementation and five example/utility programs:

- *oif_example_server*: This is an example program that implements a socket server waiting
for OIF packets. The packets are received and decoded to a Linux framebuffer device
(the code is derived from a real-world implementation).
- *oif_example_client*: This is the test client for the oif_example_server. It sends a
moving logo as overlay.
- *oif_test*: Load a logo, copy it to an overlay screen and compress it to OIF and back again.
The compression ratio is reported.
- *png2oif*: Convert a PNG file to an OIF file. With the argument -bg a background color
can be specified that is mapped to an alpha value of 0, while all other colors get an
alpha value of 255.
- *oif2png*: Convert an OIF file back to a PNG file.


## Building the Example Programs

There are only two files, implementing OIF: `oif.h` and `oif.c`. At the moment there is
no support for a dynamic or static library.

Just run make to build everything. You must have a recent version of OpenCV installed
since that is used in the example programs (not in the OIF implementation).

## Testing the Utility Programs

To convert a PNG to OIF run

  `> ./png2oif Tux-with-alpha.png`

which creates the file Tux-with-alpha.oif.
If you want to use a certain color (e.g. white) as background color (alpha = 0) run

  `> ./png2oif -bg 255,255,255 Tux-without-alpha.png`


To convert an OIF file back to PNG run

  `> cp Tux-without-alpha.oif Mytux.oif`
  
  `> ./oif2png Mytux.oif`

## Extending the format

The format has been defined with extensibility in mind. The header contains eight
reserved fields for future extensions or application specific use. This gives some
flexibility while still keeping the format and the basic functionality stable.

The source code is available under a BSD type of license.


