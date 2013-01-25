all:
	cd s2-geometry-library/geometry/; make 
	cd libevent-2.0.21-stable; make -k
	make http-server

http-server: http-server.c
	g++ -fPIC  *.c -DHAVE_CONFIG_H -I. -Ilibevent-2.0.21-stable/  -Ilibevent-2.0.21-stable/include   -O2 -Wall -fno-strict-aliasing -pthread  -Llibevent-2.0.21-stable -levent -Is2-geometry-library/geometry/ -Ls2-geometry-library/geometry/ -lgoogle-util-coding -ls2cellid -lgoogle-util-math -ls2testing -lz -lm -lssl -lgoogle-strings -lgoogle-base -L./libevent-2.0.21-stable/.libs -levent -levent_extra -levent_core -levent   -lssl -lcrypto -ls2 -lgoogle-util-coding -o http-server

http-server-new: http-server.c
	g++ -fPIC  *.c *.cpp -DHAVE_CONFIG_H -lgflags -lglog -I. -Ilibevent-2.0.21-stable/  -Ilibevent-2.0.21-stable/include   -O2 -Wall -fno-strict-aliasing -pthread  -Llibevent-2.0.21-stable -levent -Igeometry/ -Lgeometry/ -lgoogle-base -lgoogle-strings -lgoogle-util-bits -lgoogle-util-coding -lgoogle-util-geometry-s2 -lgoogle-util-geometry-s2cellid -lgoogle-util-geometry-s2testing -lgoogle-util-hash -lgoogle-util-math   -lgoogle-util-geometry-s2 -lz -lm -lssl -lgoogle-strings -lgoogle-base -L./libevent-2.0.21-stable/.libs -levent -levent_extra -levent_core -levent   -lssl -lcrypto -DHASH_NAMESPACE=__gnu_cxx \
                -DGOOGLE_NAMESPACE=base \
                              -D_START_GOOGLE_NAMESPACE_='namespace base {' \
                                            -D_END_GOOGLE_NAMESPACE_='}' -DNDEBUG -o http-server


static: http-server.c
	g++ -static -fPIC  *.c -DHAVE_CONFIG_H -I. -Ilibevent-2.0.21-stable/  -Ilibevent-2.0.21-stable/include   -O2 -Wall -fno-strict-aliasing -pthread  -Llibevent-2.0.21-stable -levent -Is2-geometry-library/geometry/ -Ls2-geometry-library/geometry/ -lrt -lgoogle-util-coding -ls2cellid -lgoogle-util-math -ls2testing -lz -lm -lssl -lgoogle-strings -lgoogle-base -L./libevent-2.0.21-stable/.libs -levent -levent_extra -levent_core -levent   -lssl -lcrypto -ls2 -lgoogle-util-coding -o http-server

clean:
	cd s2-geometry-library/geometry/; make clean
	cd libevent-2.0.21-stable; make clean
	rm http-server
