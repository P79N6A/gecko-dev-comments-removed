
























"use strict";

const EventEmitter = require("devtools/toolkit/event-emitter");
const {setTimeout, clearTimeout} = require("sdk/timers");
const {PREDEFINED, PRESETS, DEFAULT_PRESET_CATEGORY} = require("devtools/shared/widgets/CubicBezierPresets");






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
  };
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
    
    let predefName = Object.keys(PREDEFINED)
                           .find(key => coordsAreEqual(PREDEFINED[key], this.coordinates));

    return predefName || 'cubic-bezier(' + this.coordinates + ')';
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
}

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
    }];
  },

  


  offsetsToCoordinates: function(element) {
    let p = this.padding, w = this.canvas.width, h = this.canvas.height;

    
    p = p.map(function(a, i) { return a * (i % 2? w : h)});

    return [
      (parseFloat(element.style.left) - p[3]) / (w + p[1] + p[3]),
      (h - parseFloat(element.style.top) - p[2]) / (h - p[0] - p[2])
    ];
  },

  


  plot: function(settings={}) {
    let xy = this.bezier.coordinates;

    let defaultSettings = {
      handleColor: '#666',
      handleThickness: .008,
      bezierColor: '#4C9ED9',
      bezierThickness: .015,
      drawHandles: true
    };

    for (let setting in settings) {
      defaultSettings[setting] = settings[setting];
    }

    
    
    this.ctx.save();
    this.ctx.setTransform(1, 0, 0, 1, 0, 0);
    this.ctx.clearRect(0, 0, this.canvas.width, this.canvas.height);
    this.ctx.restore();

    if (defaultSettings.drawHandles) {
      
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

      var circle = function(ctx, cx, cy, r) {
        ctx.beginPath();
        ctx.arc(cx, cy, r, 0, 2*Math.PI, !1);
        ctx.closePath();
      };

      circle(this.ctx, xy[0], xy[1], 1.5 * defaultSettings.handleThickness);
      this.ctx.fill();
      circle(this.ctx, xy[2], xy[3], 1.5 * defaultSettings.handleThickness);
      this.ctx.fill();
    }

    
    this.ctx.beginPath();
    this.ctx.lineWidth = defaultSettings.bezierThickness;
    this.ctx.strokeStyle = defaultSettings.bezierColor;
    this.ctx.moveTo(0,0);
    this.ctx.bezierCurveTo(xy[0], xy[1], xy[2], xy[3], 1,1);
    this.ctx.stroke();
    this.ctx.closePath();
  }
};










function CubicBezierWidget(parent, coordinates=PRESETS["ease-in"]["ease-in-sine"]) {
  EventEmitter.decorate(this);

  this.parent = parent;
  let {curve, p1, p2} = this._initMarkup();

  this.curveBoundingBox = curve.getBoundingClientRect();
  this.curve = curve;
  this.p1 = p1;
  this.p2 = p2;

  
  this.bezierCanvas = new BezierCanvas(this.curve,
    new CubicBezier(coordinates), [0.30, 0]);
  this.bezierCanvas.plot();

  
  let offsets = this.bezierCanvas.offsets;
  this.p1.style.left = offsets[0].left;
  this.p1.style.top = offsets[0].top;
  this.p2.style.left = offsets[1].left;
  this.p2.style.top = offsets[1].top;

  this._onPointMouseDown = this._onPointMouseDown.bind(this);
  this._onPointKeyDown = this._onPointKeyDown.bind(this);
  this._onCurveClick = this._onCurveClick.bind(this);
  this._onNewCoordinates = this._onNewCoordinates.bind(this);

  
  this.presets = new CubicBezierPresetWidget(parent);

  
  this.timingPreview = new TimingFunctionPreviewWidget(parent);

  this._initEvents();
}

exports.CubicBezierWidget = CubicBezierWidget;

CubicBezierWidget.prototype = {
  _initMarkup: function() {
    let doc = this.parent.ownerDocument;

    let wrap = doc.createElement("div");
    wrap.className = "display-wrap";

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
    curve.setAttribute("width", 150);
    curve.setAttribute("height", 370);
    curve.id = "curve";

    plane.appendChild(curve);
    wrap.appendChild(plane);

    this.parent.appendChild(wrap);

    return {
      p1: p1,
      p2: p2,
      curve: curve
    };
  },

  _removeMarkup: function() {
    this.parent.ownerDocument.querySelector(".display-wrap").remove();
  },

  _initEvents: function() {
    this.p1.addEventListener("mousedown", this._onPointMouseDown);
    this.p2.addEventListener("mousedown", this._onPointMouseDown);

    this.p1.addEventListener("keydown", this._onPointKeyDown);
    this.p2.addEventListener("keydown", this._onPointKeyDown);

    this.curve.addEventListener("click", this._onCurveClick);

    this.presets.on("new-coordinates", this._onNewCoordinates);
  },

  _removeEvents: function() {
    this.p1.removeEventListener("mousedown", this._onPointMouseDown);
    this.p2.removeEventListener("mousedown", this._onPointMouseDown);

    this.p1.removeEventListener("keydown", this._onPointKeyDown);
    this.p2.removeEventListener("keydown", this._onPointKeyDown);

    this.curve.removeEventListener("click", this._onCurveClick);

    this.presets.off("new-coordinates", this._onNewCoordinates);
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
    };
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
    this.curveBoundingBox = this.curve.getBoundingClientRect();

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

  _onNewCoordinates: function(event, coordinates) {
    this.coordinates = coordinates;
  },

  


  _updateFromPoints: function() {
    
    let coordinates = this.bezierCanvas.offsetsToCoordinates(this.p1);
    coordinates = coordinates.concat(this.bezierCanvas.offsetsToCoordinates(this.p2));

    this.presets.refreshMenu(coordinates);
    this._redraw(coordinates);
  },

  



  _redraw: function(coordinates) {
    
    this.bezierCanvas.bezier = new CubicBezier(coordinates);
    this.bezierCanvas.plot();
    this.emit("updated", this.bezierCanvas.bezier);

    this.timingPreview.preview(this.bezierCanvas.bezier + "");
  },

  



  set coordinates(coordinates) {
    this._redraw(coordinates);

    
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

    this.presets.refreshMenu(coordinates);
    this.coordinates = coordinates;
  },

  destroy: function() {
    this._removeEvents();
    this._removeMarkup();

    this.timingPreview.destroy();
    this.presets.destroy();

    this.curve = this.p1 = this.p2 = null;
  }
};









function CubicBezierPresetWidget(parent) {
  this.parent = parent;

  let {presetPane, presets, categories} = this._initMarkup();
  this.presetPane = presetPane;
  this.presets = presets;
  this.categories = categories;

  this._activeCategory = null;
  this._activePresetList = null;
  this._activePreset = null;

  this._onCategoryClick = this._onCategoryClick.bind(this);
  this._onPresetClick = this._onPresetClick.bind(this);

  EventEmitter.decorate(this);
  this._initEvents();
}

exports.CubicBezierPresetWidget = CubicBezierPresetWidget;

CubicBezierPresetWidget.prototype = {
  

















  _initMarkup: function() {
    let doc = this.parent.ownerDocument;

    let presetPane = doc.createElement("div");
    presetPane.className = "preset-pane";

    let categoryList = doc.createElement("div");
    categoryList.id = "preset-categories";

    let presetContainer = doc.createElement("div");
    presetContainer.id = "preset-container";

    Object.keys(PRESETS).forEach(categoryLabel => {
      let category = this._createCategory(categoryLabel);
      categoryList.appendChild(category);

      let presetList = this._createPresetList(categoryLabel);
      presetContainer.appendChild(presetList);
    });

    presetPane.appendChild(categoryList);
    presetPane.appendChild(presetContainer);

    this.parent.appendChild(presetPane);

    let allCategories = presetPane.querySelectorAll(".category");
    let allPresets = presetPane.querySelectorAll(".preset");

    return {
      presetPane: presetPane,
      presets: allPresets,
      categories: allCategories
    };
  },

  _createCategory: function(categoryLabel) {
    let doc = this.parent.ownerDocument;

    let category = doc.createElement("div");
    category.id = categoryLabel;
    category.classList.add("category");

    let categoryDisplayLabel = this._normalizeCategoryLabel(categoryLabel);
    category.textContent = categoryDisplayLabel;

    return category;
  },

  _normalizeCategoryLabel: function(categoryLabel) {
    return categoryLabel.replace("/-/g", " ");
  },

  _createPresetList: function(categoryLabel) {
    let doc = this.parent.ownerDocument;

    let presetList = doc.createElement("div");
    presetList.id = "preset-category-" + categoryLabel;
    presetList.classList.add("preset-list");

    Object.keys(PRESETS[categoryLabel]).forEach(presetLabel => {
      let preset = this._createPreset(categoryLabel, presetLabel);
      presetList.appendChild(preset);
    });

    return presetList;
  },

  _createPreset: function(categoryLabel, presetLabel) {
    let doc = this.parent.ownerDocument;

    let preset = doc.createElement("div");
    preset.classList.add("preset");
    preset.id = presetLabel;
    preset.coordinates = PRESETS[categoryLabel][presetLabel];
    
    let curve = doc.createElement("canvas");
    let bezier = new CubicBezier(preset.coordinates);
    curve.setAttribute("height", 50);
    curve.setAttribute("width", 50);
    preset.bezierCanvas = new BezierCanvas(curve, bezier, [0.15, 0]);
    preset.bezierCanvas.plot({
      drawHandles: false,
      bezierThickness: 0.025
    });
    preset.appendChild(curve);

    
    let presetLabelElem = doc.createElement("p");
    let presetDisplayLabel = this._normalizePresetLabel(categoryLabel, presetLabel);
    presetLabelElem.textContent = presetDisplayLabel;
    preset.appendChild(presetLabelElem);

    return preset;
  },

  _normalizePresetLabel: function(categoryLabel, presetLabel) {
    return presetLabel.replace(categoryLabel + "-", "").replace("/-/g", " ");
  },

  _initEvents: function() {
    for (let category of this.categories) {
      category.addEventListener("click", this._onCategoryClick);
    }

    for (let preset of this.presets) {
      preset.addEventListener("click", this._onPresetClick);
    }
  },

  _removeEvents: function() {
    for (let category of this.categories) {
      category.removeEventListener("click", this._onCategoryClick);
    }

    for (let preset of this.presets) {
      preset.removeEventListener("click", this._onPresetClick);
    }
  },

  _onPresetClick: function(event) {
    this.emit("new-coordinates", event.currentTarget.coordinates);
    this.activePreset = event.currentTarget;
  },

  _onCategoryClick: function(event) {
    this.activeCategory = event.target;
  },

  _setActivePresetList: function(presetListId) {
    let presetList = this.presetPane.querySelector("#" + presetListId);
    swapClassName("active-preset-list", this._activePresetList, presetList);
    this._activePresetList = presetList;
  },

  set activeCategory(category) {
    swapClassName("active-category", this._activeCategory, category);
    this._activeCategory = category;
    this._setActivePresetList("preset-category-" + category.id);
  },

  get activeCategory() {
    return this._activeCategory;
  },

  set activePreset(preset) {
    swapClassName("active-preset", this._activePreset, preset);
    this._activePreset = preset;
  },

  get activePreset() {
    return this._activePreset;
  },

  






  refreshMenu: function(coordinates) {
    
    
    let category = this._activeCategory;

    
    
    let preset = null;

    
    
    if (!category) {
      category = this.parent.querySelector("#" + DEFAULT_PRESET_CATEGORY);
    }

    
    
    Object.keys(PRESETS).forEach(categoryLabel => {

      Object.keys(PRESETS[categoryLabel]).forEach(presetLabel => {
        if (coordsAreEqual(PRESETS[categoryLabel][presetLabel], coordinates)) {
          category = this.parent.querySelector("#" + categoryLabel);
          preset = this.parent.querySelector("#" + presetLabel);
        }
      });

    });

    this.activeCategory = category;
    this.activePreset = preset;
  },

  destroy: function() {
    this._removeEvents();
    this.parent.querySelector(".preset-pane").remove();
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







function swapClassName(className, from, to) {
  if (from !== null) {
    from.classList.remove(className);
  }

  if (to !== null) {
    to.classList.add(className);
  }
}







function coordsAreEqual(c1, c2) {
  return c1.reduce((prev, curr, index) => prev && (curr === c2[index]), true);
}
