#!/bin/sh
g++ -DHAVE_CONFIG_H -I. -I..  -I.. -I../compat -I../include -I../include   -g -O2 -Wall -fno-strict-aliasing -pthread  -L../ -levent -I../../s2-geometry-library/geometry/ -L../../s2-geometry-library/geometry/ -lrt -lgoogle-util-coding -ls2cellid -lgoogle-util-math -ls2testing -lz -lm -lssl -lgoogle-strings -lgoogle-base http-server.c
