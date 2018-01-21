Overview
========
S2Map is two things
- A simple geo visualizer for when you just need to look at some points on a map quickly. 
- A suite of visualizations for s2 cells and coverings. [S2](http://code.google.com/p/s2-geometry-library/) is the google quadtree/spatial curve that powers google maps, foursquare and mongodb.

Inspiration
===========
Often I find I have one or more lat/lngs, and I want to put them on a map and not worry about formatting, writing a file, etc, so I just paste it into s2map.com

Usage Examples
==============
- example of pasting in a bounding box from some json: http://cl.ly/image/1n2o1p0Y2e0A
- using it via url params (doesn't support all the options yet): http://s2map.com/#40.74,-74,40.75,-74.05
- rendering some geojson coordinates in lng/lat format: http://cl.ly/image/1s12263h0X3r
- oh, and just for kicks alt/ctrl/meta+click pops up a balloon with lat/lng info

Running Locally
===============
There is a python server that uses the python implementation of s2, it does not currently support coverings, but does support much of the other functionality.

It can be run as follows:

- Install python 2
- virtualenv venv
- source venv/bin/activate
- python app.py
- open http://localhost:5000

The version on s2map.com uses a combination of the python server for serving the frontend files (and really nothing else) and the C++ version for the API. I do this with a docker setup that compiles the C++ binary, installs the python dependencies, and chains them together with a simple nginx proxy. You can replicate this yourself, or work with docker

- docker build -t s2map .
- docker run -p 81:5000 --name s2map -t s2map
- open http://localhost:5000

Errata
======
I'm curious if other people find this useful or have feature suggestions.

TODO
====
- See about making it 100% python for ease of not compiling C++