#!/bin/sh
/etc/init.d/nginx start
./s2map-server/http-server 3001 &
python ./frontend/app.py

