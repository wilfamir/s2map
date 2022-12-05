# -*- coding: utf-8 -*-
import logging
import s2sphere
import os
from flask import Flask, send_from_directory
from flask import jsonify
from flask import make_response
from functools import update_wrapper
from datetime import datetime
from functools import wraps
from flask import request, current_app


logging.basicConfig()


def nocache(view):
    @wraps(view)
    def no_cache(*args, **kwargs):
        response = make_response(view(*args, **kwargs))
        response.headers['Last-Modified'] = datetime.now()
        response.headers['Cache-Control'] = 'no-store, no-cache, must-revalidate, post-check=0, pre-check=0, max-age=0'
        response.headers['Pragma'] = 'no-cache'
        response.headers['Expires'] = '-1'
        return response

    return update_wrapper(no_cache, view)


app = Flask(__name__)


def cell_id_to_json(cellid_l):
    cellid = s2sphere.CellId(cellid_l)
    cell = s2sphere.Cell(cellid)

    def get_vertex(v):
        vertex = s2sphere.LatLng.from_point(cell.get_vertex(v))

        return {
            'lat': vertex.lat().degrees,
            'lng': vertex.lng().degrees
        }

    shape = [get_vertex(v) for v in range(0, 4)]
    return {
        'id': str(cellid.id()),
        'id_signed': cellid.id(),
        'token': cellid.to_token(),
        'pos': cellid.pos(),
        'face': cellid.face(),
        'level': cellid.level(),
        'll': {
            'lat': cellid.to_lat_lng().lat().degrees,
            'lng': cellid.to_lat_lng().lng().degrees
        },
        'shape': shape
    }


def jsonp(func):
    """Wraps JSONified output for JSONP requests."""
    @wraps(func)
    def decorated_function(*args, **kwargs):
        callback = request.args.get('callback', False)
        if callback:
            data = str(func(*args, **kwargs).data)
            content = str(callback) + '(' + data + ')'
            mimetype = 'application/javascript'
            return current_app.response_class(content, mimetype=mimetype)
        else:
            return func(*args, **kwargs)
    return decorated_function


@app.route('/api/s2info', methods=['GET', 'POST'])
@jsonp
def s2info():
    # need to make this work for GET and POST
    ids = request.args.get("id") or request.form['id'] or ''
    return jsonify([cell_id_to_json(int(id)) for id in ids.split(',')])


@nocache
@app.route('/<path:path>')
def send_app(path):
    print('trying to send ' + path)
    return send_from_directory('.', path)


@nocache
@app.route('/')
def send_index():
    print('sending index?')
    return send_from_directory('.', "index.html")


@nocache
@app.route('/ind')
def send_i2ndex():
    return send_from_directory('.', "index.html")


if __name__ == '__main__':
    app.run(debug=True, use_reloader=True, host=os.getenv("APP_HOST", "127.0.0.1"))
