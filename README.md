I'm slowly building out s2map.com into a simple geo visualizer for when you just need to look at some points on a map quickly. The s2 in the name comes from the google s2 geometry library, which is what our internal version at foursquare visualizes: http://cl.ly/image/3J200M0a2511 -- I'm working on a public version based on google's c++ s2 implementation http://code.google.com/p/s2-geometry-library/

ANYWAY

often I find I have one or more lat/lngs, and I want to put them on a map and not worry about formatting, writing a file, etc, so I just paste it into s2map.com

example of pasting in a bounding box from some json: http://cl.ly/image/1n2o1p0Y2e0A
using it via url params (doesn't support all the options yet): http://s2map.com/#40.74,-74,40.75,-74.05
rendering some geojson coordinates in lng/lat format: http://cl.ly/image/1s12263h0X3r 

I'm curious if other people find this useful or have feature suggestions.

oh, and just for kicks alt/ctrl/meta+click pops up a balloon with lat/lng info
