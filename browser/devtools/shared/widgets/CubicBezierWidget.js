
























"use strict";

const EventEmitter = require("devtools/toolkit/event-emitter");
const {setTimeout, clearTimeout} = require("sdk/timers");

const PREDEFINED = exports.PREDEFINED = {
  "ease": [.25, .1, .25, 1],
  "linear": [0, 0, 1, 1],
  "ease-in": [.42, 0, 1, 1],
  "ease-out": [0, 0, .58, 1],
  "ease-in-out": [.42, 0, .58, 1]
};






function CubicBezier(coordinates) {
  if (!coordinates) {
    throw "No offsets were defined";
  }

  this.coordinates = coordinates.map(n => +n);

  for (let i = 4; i--;) {
    let xy = this.coordinates[i];
    if (isNaN(xy) || (!(i%2) && (xy < 0 || xy > 1))) {
      throw "Wrong coordinate at " + i + "(" + xy + ")";
    }
  }

  this.coordinates.toString = function() {
    return this.map(n => {
      return (Math.round(n * 100)/100 + '').replace(/^0\./, '.');
    }) + "";
  }
}

exports.CubicBezier = CubicBezier;

CubicBezier.prototype = {
  get P1() {
    return this.coordinates.slice(0, 2);
  },

  get P2() {
    return this.coordinates.slice(2);
  },

  toString: function() {
    return 'cubic-bezier(' + this.coordinates + ')';
  }
};







function BezierCanvas(canvas, bezier, padding) {
  this.canvas = canvas;
  this.bezier = bezier;
  this.padding = getPadding(padding);

  
  this.ctx = this.canvas.getContext('2d');
  let p = this.padding;

  this.ctx.scale(canvas.width * (1 - p[1] - p[3]),
                 -canvas.height * (1 - p[0] - p[2]));
  this.ctx.translate(p[3] / (1 - p[1] - p[3]),
                     -1 - p[0] / (1 - p[0] - p[2]));
};

exports.BezierCanvas = BezierCanvas;

BezierCanvas.prototype = {
  



  get offsets() {
    let p = this.padding, w = this.canvas.width, h = this.canvas.height;

    return [{
      left: w * (this.bezier.coordinates[0] * (1 - p[3] - p[1]) - p[3]) + 'px',
      top: h * (1 - this.bezier.coordinates[1] * (1 - p[0] - p[2]) - p[0]) + 'px'
    }, {
      left: w * (this.bezier.coordinates[2] * (1 - p[3] - p[1]) - p[3]) + 'px',
      top: h * (1 - this.bezier.coordinates[3] * (1 - p[0] - p[2]) - p[0]) + 'px'
    }]
  },

  


  offsetsToCoordinates: function(element) {
    let p = this.padding, w = this.canvas.width, h = this.canvas.height;

    
    p = p.map(function(a, i) { return a * (i % 2? w : h)});

    return [
      (parseInt(element.style.left) - p[3]) / (w + p[1] + p[3]),
      (h - parseInt(element.style.top) - p[2]) / (h - p[0] - p[2])
    ];
  },

  


  plot: function(settings={}) {
    let xy = this.bezier.coordinates;

    let defaultSettings = {
      handleColor: '#666',
      handleThickness: .008,
      bezierColor: '#4C9ED9',
      bezierThickness: .015
    };

    for (let setting in settings) {
      defaultSettings[setting] = settings[setting];
    }

    this.ctx.clearRect(-.5,-.5, 2, 2);

    
    this.ctx.beginPath();
    this.ctx.fillStyle = defaultSettings.handleColor;
    this.ctx.lineWidth = defaultSettings.handleThickness;
    this.ctx.strokeStyle = defaultSettings.handleColor;

    this.ctx.moveTo(0, 0);
    this.ctx.lineTo(xy[0], xy[1]);
    this.ctx.moveTo(1,1);
    this.ctx.lineTo(xy[2], xy[3]);

    this.ctx.stroke();
    this.ctx.closePath();

    function circle(ctx, cx, cy, r) {
      return ctx.beginPath();
      ctx.arc(cx, cy, r, 0, 2*Math.PI, !1);
      ctx.closePath();
    }

    circle(this.ctx, xy[0], xy[1], 1.5 * defaultSettings.handleThickness);
    this.ctx.fill();
    circle(this.ctx, xy[2], xy[3], 1.5 * defaultSettings.handleThickness);
    this.ctx.fill();

    
    this.ctx.beginPath();
    this.ctx.lineWidth = defaultSettings.bezierThickness;
    this.ctx.strokeStyle = defaultSettings.bezierColor;
    this.ctx.moveTo(0,0);
    this.ctx.bezierCurveTo(xy[0], xy[1], xy[2], xy[3], 1,1);
    this.ctx.stroke();
    this.ctx.closePath();
  }
};










function CubicBezierWidget(parent, coordinates=PREDEFINED["ease-in-out"]) {
  this.parent = parent;
  let {curve, p1, p2} = this._initMarkup();

  this.curve = curve;
  this.curveBoundingBox = curve.getBoundingClientRect();
  this.p1 = p1;
  this.p2 = p2;

  
  this.bezierCanvas = new BezierCanvas(this.curve,
    new CubicBezier(coordinates), [.25, 0]);
  this.bezierCanvas.plot();

  
  let offsets = this.bezierCanvas.offsets;
  this.p1.style.left = offsets[0].left;
  this.p1.style.top = offsets[0].top;
  this.p2.style.left = offsets[1].left;
  this.p2.style.top = offsets[1].top;

  this._onPointMouseDown = this._onPointMouseDown.bind(this);
  this._onPointKeyDown = this._onPointKeyDown.bind(this);
  this._onCurveClick = this._onCurveClick.bind(this);
  this._initEvents();

  
  this.timingPreview = new TimingFunctionPreviewWidget(parent);

  EventEmitter.decorate(this);
}

exports.CubicBezierWidget = CubicBezierWidget;

CubicBezierWidget.prototype = {
  _initMarkup: function() {
    let doc = this.parent.ownerDocument;

    let plane = doc.createElement("div");
    plane.className = "coordinate-plane";

    let p1 = doc.createElement("button");
    p1.className = "control-point";
    p1.id = "P1";
    plane.appendChild(p1);

    let p2 = doc.createElement("button");
    p2.className = "control-point";
    p2.id = "P2";
    plane.appendChild(p2);

    let curve = doc.createElement("canvas");
    curve.setAttribute("height", "400");
    curve.setAttribute("width", "200");
    curve.id = "curve";
    plane.appendChild(curve);

    this.parent.appendChild(plane);

    return {
      p1: p1,
      p2: p2,
      curve: curve
    }
  },

  _removeMarkup: function() {
    this.parent.ownerDocument.querySelector(".coordinate-plane").remove();
  },

  _initEvents: function() {
    this.p1.addEventListener("mousedown", this._onPointMouseDown);
    this.p2.addEventListener("mousedown", this._onPointMouseDown);

    this.p1.addEventListener("keydown", this._onPointKeyDown);
    this.p2.addEventListener("keydown", this._onPointKeyDown);

    this.curve.addEventListener("click", this._onCurveClick);
  },

  _removeEvents: function() {
    this.p1.removeEventListener("mousedown", this._onPointMouseDown);
    this.p2.removeEventListener("mousedown", this._onPointMouseDown);

    this.p1.removeEventListener("keydown", this._onPointKeyDown);
    this.p2.removeEventListener("keydown", this._onPointKeyDown);

    this.curve.removeEventListener("click", this._onCurveClick);
  },

  _onPointMouseDown: function(event) {
    
    this.curveBoundingBox = this.curve.getBoundingClientRect();

    let point = event.target;
    let doc = point.ownerDocument;
    let self = this;

    doc.onmousemove = function drag(e) {
      let x = e.pageX;
      let y = e.pageY;
      let left = self.curveBoundingBox.left;
      let top = self.curveBoundingBox.top;

      if (x === 0 && y == 0) {
        return;
      }

      
      x = Math.min(Math.max(left, x), left + self.curveBoundingBox.width);

      point.style.left = x - left + "px";
      point.style.top = y - top + "px";

      self._updateFromPoints();
    };

    doc.onmouseup = function () {
      point.focus();
      doc.onmousemove = doc.onmouseup = null;
    }
  },

  _onPointKeyDown: function(event) {
    let point = event.target;
    let code = event.keyCode;

    if (code >= 37 && code <= 40) {
      event.preventDefault();

      
      let left = parseInt(point.style.left);
      let top = parseInt(point.style.top);
      let offset = 3 * (event.shiftKey ? 10 : 1);

      switch (code) {
        case 37: point.style.left = left - offset + 'px'; break;
        case 38: point.style.top = top - offset + 'px'; break;
        case 39: point.style.left = left + offset + 'px'; break;
        case 40: point.style.top = top + offset + 'px'; break;
      }

      this._updateFromPoints();
    }
  },

  _onCurveClick: function(event) {
    let left = this.curveBoundingBox.left;
    let top = this.curveBoundingBox.top;
    let x = event.pageX - left;
    let y = event.pageY - top;

    
    let distP1 = distance(x, y,
      parseInt(this.p1.style.left), parseInt(this.p1.style.top));
    let distP2 = distance(x, y,
      parseInt(this.p2.style.left), parseInt(this.p2.style.top));

    let point = distP1 < distP2 ? this.p1 : this.p2;
    point.style.left = x + "px";
    point.style.top = y + "px";

    this._updateFromPoints();
  },

  


  _updateFromPoints: function() {
    
    let coordinates = this.bezierCanvas.offsetsToCoordinates(this.p1)
    coordinates = coordinates.concat(this.bezierCanvas.offsetsToCoordinates(this.p2));

    this._redraw(coordinates);
  },

  



  _redraw: function(coordinates) {
    
    this.bezierCanvas.bezier = new CubicBezier(coordinates);
    this.bezierCanvas.plot();
    this.emit("updated", this.bezierCanvas.bezier);

    this.timingPreview.preview(this.bezierCanvas.bezier + "");
  },

  



  set coordinates(coordinates) {
    this._redraw(coordinates)

    
    let offsets = this.bezierCanvas.offsets;
    this.p1.style.left = offsets[0].left;
    this.p1.style.top = offsets[0].top;
    this.p2.style.left = offsets[1].left;
    this.p2.style.top = offsets[1].top;
  },

  



  set cssCubicBezierValue(value) {
    if (!value) {
      return;
    }

    value = value.trim();

    
    let coordinates = PREDEFINED[value];

    
    if (!coordinates && value.startsWith("cubic-bezier")) {
      coordinates = value.replace(/cubic-bezier|\(|\)/g, "").split(",").map(parseFloat);
    }

    this.coordinates = coordinates;
  },

  destroy: function() {
    this._removeEvents();
    this._removeMarkup();

    this.timingPreview.destroy();

    this.curve = this.p1 = this.p2 = null;
  }
};






function TimingFunctionPreviewWidget(parent) {
  this.previousValue = null;
  this.autoRestartAnimation = null;

  this.parent = parent;
  this._initMarkup();
}

TimingFunctionPreviewWidget.prototype = {
  PREVIEW_DURATION: 1000,

  _initMarkup: function() {
    let doc = this.parent.ownerDocument;

    let container = doc.createElement("div");
    container.className = "timing-function-preview";

    this.dot = doc.createElement("div");
    this.dot.className = "dot";
    container.appendChild(this.dot);

    let scale = doc.createElement("div");
    scale.className = "scale";
    container.appendChild(scale);

    this.parent.appendChild(container);
  },

  destroy: function() {
    clearTimeout(this.autoRestartAnimation);
    this.parent.querySelector(".timing-function-preview").remove();
    this.parent = this.dot = null;
  },

  





  preview: function(value) {
    
    if (value === this.previousValue) {
      return false;
    }

    clearTimeout(this.autoRestartAnimation);

    if (isValidTimingFunction(value)) {
      this.dot.style.animationTimingFunction = value;
      this.restartAnimation();
    }

    this.previousValue = value;
  },

  


  restartAnimation: function() {
    
    this.dot.style.animationDuration = (this.PREVIEW_DURATION * 2) + "ms";

    
    this.dot.classList.remove("animate");
    let w = this.dot.offsetWidth;
    this.dot.classList.add("animate");

    
    this.autoRestartAnimation = setTimeout(this.restartAnimation.bind(this),
      this.PREVIEW_DURATION * 2);
  }
};



function getPadding(padding) {
  let p = typeof padding === 'number'? [padding] : padding;

  if (p.length === 1) {
    p[1] = p[0];
  }

  if (p.length === 2) {
    p[2] = p[0];
  }

  if (p.length === 3) {
    p[3] = p[1];
  }

  return p;
}

function distance(x1, y1, x2, y2) {
  return Math.sqrt(Math.pow(x1 - x2, 2) + Math.pow(y1 - y2, 2));
}






function isValidTimingFunction(value) {
  
  if (value in PREDEFINED) {
    return true;
  }

  
  if (value.match(/^cubic-bezier\(([0-9.\- ]+,){3}[0-9.\- ]+\)/)) {
    return true;
  }

  return false;
}
