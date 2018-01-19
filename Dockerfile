FROM ubuntu:17.10
RUN apt-get update
RUN apt-get -y install make g++ swig libssl-dev cmake gawk libevent-dev libcurl4-openssl-dev libboost-dev nginx
COPY s2map-server/ /usr/src/myapp/s2map-server
COPY nginx.conf /etc/nginx/sites-enabled/proxy.conf
WORKDIR /usr/src/myapp/s2map-server
RUN make
RUN ldconfig
RUN apt-get -y install python-pip
COPY frontend/ /usr/src/myapp/frontend
WORKDIR /usr/src/myapp/frontend
RUN pip install -r requirements.txt
COPY run-all.sh /usr/src/myapp/
WORKDIR /usr/src/myapp
RUN apt-get -y install curl less vim
RUN rm /etc/nginx/sites-enabled
CMD ["/usr/src/myapp/run-all.sh"]
EXPOSE 80
