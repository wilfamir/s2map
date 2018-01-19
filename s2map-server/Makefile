all:
	cd s2-geometry-library/geometry/; cd geometry; cmake .; make -j3; make install
	make http-server

http-server: http-server.c
	g++ -fPIC  *.c -DHAVE_CONFIG_H -I. -Ilibevent-2.0.21-stable/  -Ilibevent-2.0.21-stable/include   -O2 -Wall -fno-strict-aliasing -pthread  -lcurl -levent -Llibevent-2.0.21-stable -I/usr/local/include/s2 -levent -levent_extra -levent_core -levent   -lssl -lcrypto -ls2cellid -ls2util -ls2 -o http-server

clean:
	cd s2-geometry-library/geometry/; make clean
	cd libevent-2.0.21-stable; make clean
	rm http-server
