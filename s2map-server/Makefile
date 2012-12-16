all:
	cd s2-geometry-library/geometry/; make 
	cd libevent-2.0.21-stable; make
	g++ -fPIC  http-server.c -DHAVE_CONFIG_H -I. -Ilibevent-2.0.21-stable/  -Ilibevent-2.0.21-stable/include   -O2 -Wall -fno-strict-aliasing -pthread  -Llibevent-2.0.21-stable -levent -Is2-geometry-library/geometry/ -Ls2-geometry-library/geometry/ -lrt -lgoogle-util-coding -ls2cellid -lgoogle-util-math -ls2testing -lz -lm -lssl -lgoogle-strings -lgoogle-base -L./libevent-2.0.21-stable/.libs -levent -levent_extra -levent_core -levent   -lssl -lcrypto -ls2 -lgoogle-util-coding -o http-server

clean:
	cd s2-geometry-library/geometry/; make clean
	cd libevent-2.0.21-stable; make clean
	rm http-server
