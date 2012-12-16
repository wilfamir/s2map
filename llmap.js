var PageController = Backbone.Model.extend({
/**
 * The earth's radius in meters
 * @constant
 * @type {number}
 */
EARTH_RADIUS_M: 6371 * 1000,

/**
 * @param {number} degrees
 * @return {number}
 */
degreesToRadians: function(degrees) {
  return degrees * 0.0174532925;
},

/**
 * @param {L.LatLng} pointA
 * @param {L.LatLng} pointB
 * @return {number}
 */
distanceBetween: function(pointA, pointB) {
  var latRadA = this.degreesToRadians(pointA.lat);
  var latRadB = this.degreesToRadians(pointB.lat);
  var lngRadA = this.degreesToRadians(pointA.lng);
  var lngRadB = this.degreesToRadians(pointB.lng);

  return Math.acos(Math.sin(latRadA) * Math.sin(latRadB) +
         Math.cos(latRadA) * Math.cos(latRadB) * Math.cos(lngRadA - lngRadB)) * this.EARTH_RADIUS_M;
},

/*
 * @returns {bool}
 */
isReverseOrder: function() {
  return this.$reverseOrder.is(':checked');
},

/*
 * @returns {bool}
 */
inPolygonMode: function() {
  return this.$polygonMode.is(':checked');
},

/*
 * @returns {bool}
 */
inPointMode: function() {
  return this.$pointMode.is(':checked');
},


shouldClear: function() {
  return this.$clearButton.is(':checked');
},

/*
 * @returns {bool}
 */
inLineMode: function() {
  return this.$lineMode.is(':checked');
},

resetDisplay: function() {
  if (this.shouldClear()) {
    this.layerGroup.clearLayers();
  }
  this.$infoArea.empty();
},

addInfo: function(msg) {
  this.$infoArea.append($('<div>' + msg + '</div>'));
},

getPoints: function(tokens) {
  var points = [];
  if (!tokens) {
    return points;
  }

  var isReverseOrder = this.isReverseOrder();
  _(_.range(0, tokens.length, 2)).each(function(i) {
    if (isReverseOrder) {
      points.push(
        new L.LatLng(tokens[i+1], tokens[i])
      );
    } else {
      points.push(
        new L.LatLng(tokens[i], tokens[i+1])
      );
    }
  });
  return points;
},

  cellDescription: function(cell) {
    return cell.id + ' ' + cell.token + ' : ' + cell.description + ' ('
      + cell.ll.lat + ',' + cell.ll.lng + ')';
  },

  /** 
   * @param {fourSq.api.models.geo.S2Response} cell
   * @return {L.Polygon}
   */
  renderCell: function(cell) {
    var description = this.cellDescription(cell)
    this.$infoArea.append(description);
    this.$infoArea.append('<br/>');


    var points = _(cell.shape).map(function(ll) {
      return new L.LatLng(ll.lat, ll.lng);
    });

    var polygon = new L.Polygon(points,
      { 
        color: "#ff0000",
        weight: 1,
        fill: true,
        fillOpacity: 0.2
      });
    polygon.bindPopup(description);

    this.layerGroup.addLayer(polygon);
    return polygon;
  },

  /** 
   * @param {Array.<fourSq.api.models.geo.S2Response>} cells
   * @return {Array.<L.Polygon>}
   */
  renderCells: function(cells) {
    return _(cells).map(_.bind(function(c) {
      return this.renderCell(c);
    }, this));
  },

  idsCallback: function() {
    this.resetDisplay();
    function render(cells) {
      var bounds = null;
      var polygons = this.renderCells(cells);
      _.each(polygons, function(p) {
        if (!bounds) {
          bounds = new L.LatLngBounds([p.getBounds().getCenter()]);
        }
        bounds.extend(p.getBounds().getCenter());
      });
      this.map.fitBounds(bounds);
    }

    var ids = this.$boundsInput.val()
      .replace(/^\s+/g, '')
      .replace(/ /g, ',')
      .replace(/\n/g, ',')
      .replace(/[^\w\s\.\-\,]|_/g, '');

    var idList = ids.split(',')
    console.log(idList)
    var size = 75
    _.range(0, idList.length, size).map(_.bind(function(start) {
      $.ajax({
        url: 'http://api.s2map.com/s2info?callback=?',
        dataType: 'json',
        data: {
          'id': idList.slice(start, start+size).join(',')
        },
        success: _.bind(render, this)
      });
    }, this));
  },

renderMarkers: function(points) {
  this.resetDisplay();

  var bounds = new L.LatLngBounds(_.map(points, function(p) {
    return p.getLatLng();
  }));

  _.each(points, _.bind(function(p) {
    this.layerGroup.addLayer(p);
  }, this));
  
  this.processBounds(bounds);
},

processBounds: function(bounds) {
  if (!this.shouldClear() && !!this.previousBounds) {
    bounds = this.previousBounds.extend(bounds)
  }
  this.previousBounds = bounds;

  var zoom = this.map.getBoundsZoom(bounds) - 1;      

  // TODO: add control offset logic?
  var centerPixel = this.map.project(bounds.getCenter(), zoom);
  var centerPoint = this.map.unproject(centerPixel, zoom)
  this.map.setView(centerPoint, zoom);
},

renderPolygon: function(polygon, bounds) {
  this.resetDisplay();

  this.layerGroup.addLayer(polygon);

  this.processBounds(bounds);
},

boundsCallback: function() {
  var bboxstr = this.$boundsInput.val() || this.placeholder;

  var regex = /[+-]?\d+\.\d+/g;
  var bboxParts = bboxstr.match(regex);

  var points = this.getPoints(bboxParts);

  var polygonPoints = []
  if (points.length == 0) {
    // try s2 parsing!
    this.idsCallback();
    return;
  }

  if (points.length == 1) {
    var ll = points[0];
    this.map.setView(ll, 15);
    var marker = new L.Marker(ll);
    this.renderMarkers([marker]);
  } else if (this.inPolygonMode()) {
    if (points.length == 2) {
       var ll1 = points[0]
       var ll2 = points[1]
       var bounds = new L.LatLngBounds(ll1, ll2);

      var ne = bounds.getNorthEast();
      var sw = bounds.getSouthWest();
      var nw = new L.LatLng(ne.lat, sw.lng);
      var se = new L.LatLng(sw.lat, ne.lng);

      polygonPoints = [nw, ne, se, sw];
    } else {
      polygonPoints = points; 
    }

    var polygon = new L.Polygon(polygonPoints,  
       {color: "#0000ff", weight: 1, fill: true, fillOpacity: 0.2});
    this.renderPolygon(polygon, polygon.getBounds())
  } else if (this.inLineMode()) {
    var polyline = new L.Polyline(points,  
     {color: "#0000ff", weight: 4, fill: false, fillOpacity: 0.2});
    this.renderPolygon(polyline, polyline.getBounds());

    _.each(_.range(0, points.length - 1), _.bind(function(index) {
      var a = points[index];
      var b = points[(index+1) % points.length];
      var distance = this.distanceBetween(a, b);
      this.addInfo(a + ' --> ' + b + '<br/>--- distance: ' + distance + 'm');
    }, this))
  } else if (this.inPointMode()) {
    var markers = _.map(points, function(p) {
      return new L.Marker(p);
    });
    this.renderMarkers(markers);
  }
 
  // fourSq.api.services.Geo.s2cover({
  //     ne: ne.lat + ',' + ne.lng,
  //     sw: sw.lat + ',' + sw.lng
  //   },
  //   _.bind(this.renderCells, this)
  // );
},

initialize: function() {
  var mapUrl = 'http://{s}.tiles.mapbox.com/v3/mapbox.mapbox-streets/{z}/{x}/{y}.png';
  var subdomains = ['a','b','c','d'];

  var opts = {
    layers: new L.TileLayer(mapUrl, {subdomains: subdomains}),
    attributionControl: true,
    zoomControl: false
  }

  this.map = new L.Map('map', opts);
  var zoom = new L.Control.Zoom()
  zoom.setPosition('topright');
  this.map.addControl(zoom);

  this.layerGroup = new L.LayerGroup();
  this.map.addLayer(this.layerGroup);

  this.map.on('click', _.bind(function(e) {
    if (e.originalEvent.metaKey ||
        e.originalEvent.altKey ||
        e.originalEvent.ctrlKey) {
      var popup = L.popup()
        .setLatLng(e.latlng)
        .setContent(e.latlng.lat + ',' + e.latlng.lng)
        .openOn(this.map);
    }
  }, this));

  this.$el = $(document);
  this.$infoArea = this.$el.find('.info');

  this.$reverseOrder = this.$el.find('.lnglatMode');

  this.$lineMode = this.$el.find('.lineMode');
  this.$polygonMode = this.$el.find('.polygonMode');
  this.$pointMode = this.$el.find('.pointMode');

  this.$boundsButton = this.$el.find('.boundsButton');
  this.$boundsInput = this.$el.find('.boundsInput');

  this.$clearButton = this.$el.find('.clearMap');

  this.$boundsButton.click(_.bind(this.boundsCallback, this));
  this.$boundsInput.keypress(/** @param {jQuery.Event} e */ _.bind(function(e) {
    // search on enter only
    if (e.which == 13) {
      // this.boundsCallback();
    }
  }, this));

  var placeholders = [
   '40.74,-74.0',
   '40.74,-74.0,40.75,-74.1',
   'bbox: { \n' +
   '  ne: { ' +
   '     lat: 40.74,' +
   '     lng: -74.0' +
   '   },' +
   '   sw: {' +
   '     lat: 40.75, ' +
   '     lng: -74.1 ' +
   '   }, ' +
   ' }',
  ];

  this.placeholder = _.first(_.shuffle(placeholders));
  this.$boundsInput.attr('placeholder', this.placeholder);

  var points = window.location.hash.substring(1)
  if (!!points) {
    this.$boundsInput.val(points);
  }
  this.boundsCallback();
}

});

