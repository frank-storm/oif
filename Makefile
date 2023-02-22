#
# Copyright (C) 2023 by Frank Storm <frank.storm@storm-se.com>
#
# Permission to use, copy, modify, and/or distribute this software
# for any purpose with or without fee is hereby granted.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
# WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL
# THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR
# CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
# FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
# CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
#

CXX = g++
CC = gcc

FLAGS = -O3 -Wall

INCS = $(shell pkg-config --cflags opencv)

LIBS = $(shell pkg-config --libs opencv)


OBJS = oif.o

all: png2oif oif2png oif_example_server oif_example_client oif_test


oif_test: oif_test.cpp $(OBJS) oif.h
	$(CXX) $(INCS) $(FLAGS) $(INCS) -o oif_test oif_test.cpp $(OBJS) $(LIBS)

png2oif: png2oif.cpp $(OBJS) oif.h
	$(CXX) $(FLAGS) $(INCS) -o png2oif png2oif.cpp $(OBJS) $(LIBS)

oif2png: oif2png.cpp $(OBJS) oif.h
	$(CXX) $(FLAGS) $(INCS) -o oif2png oif2png.cpp $(OBJS) $(LIBS)

oif_example_server: oif_example_server.c $(OBJS) oif.h
	$(CXX) $(FLAGS) $(INCS) -o oif_example_server oif_example_server.c $(OBJS) $(LIBS)

oif_example_client: oif_example_client.cpp $(OBJS) oif.h
	$(CXX) $(FLAGS) $(INCS) -o oif_example_client oif_example_client.cpp $(OBJS) $(LIBS)


oif.o: oif.c oif.h
	$(CXX) $(FLAGS) -c oif.c

clean:
	- rm *.o oif_test png2oif oif2png oif_example_server oif_example_client







