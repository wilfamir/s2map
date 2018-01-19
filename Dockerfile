FROM ubuntu:17.10
RUN apt-get update
RUN apt-get -y install make g++ swig libssl-dev cmake gawk libevent-dev libcurl4-openssl-dev libboost-dev
COPY . /usr/src/myapp
WORKDIR /usr/src/myapp/s2map-server
RUN make
CMD ["./http-server"]
