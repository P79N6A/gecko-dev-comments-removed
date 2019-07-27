


"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");

const { Task } = Cu.import("resource://gre/modules/Task.jsm", {});
const { Heritage, setNamedTimeout, clearNamedTimeout } = require("resource:///modules/devtools/ViewHelpers.jsm");

loader.lazyRequireGetter(this, "promise");
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");

loader.lazyImporter(this, "DevToolsWorker",
  "resource://gre/modules/devtools/shared/worker.js");
loader.lazyImporter(this, "LayoutHelpers",
  "resource://gre/modules/devtools/LayoutHelpers.jsm");

const HTML_NS = "http://www.w3.org/1999/xhtml";
const GRAPH_SRC = "chrome://browser/content/devtools/graphs-frame.xhtml";
const WORKER_URL = "resource:///modules/devtools/GraphsWorker.js";



const GRAPH_RESIZE_EVENTS_DRAIN = 100; 
const GRAPH_WHEEL_ZOOM_SENSITIVITY = 0.00075;
const GRAPH_WHEEL_SCROLL_SENSITIVITY = 0.1;
const GRAPH_WHEEL_MIN_SELECTION_WIDTH = 10; 

const GRAPH_SELECTION_BOUNDARY_HOVER_LINE_WIDTH = 4; 
const GRAPH_SELECTION_BOUNDARY_HOVER_THRESHOLD = 10; 
const GRAPH_MAX_SELECTION_LEFT_PADDING = 1;
const GRAPH_MAX_SELECTION_RIGHT_PADDING = 1;

const GRAPH_REGION_LINE_WIDTH = 1; 
const GRAPH_REGION_LINE_COLOR = "rgba(237,38,85,0.8)";

const GRAPH_STRIPE_PATTERN_WIDTH = 16; 
const GRAPH_STRIPE_PATTERN_HEIGHT = 16; 
const GRAPH_STRIPE_PATTERN_LINE_WIDTH = 2; 
const GRAPH_STRIPE_PATTERN_LINE_SPACING = 4; 




this.GraphCursor = function() {
  this.x = null;
  this.y = null;
};

this.GraphArea = function() {
  this.start = null;
  this.end = null;
};

this.GraphAreaDragger = function(anchor = new GraphArea()) {
  this.origin = null;
  this.anchor = anchor;
};

this.GraphAreaResizer = function() {
  this.margin = null;
};































this.AbstractCanvasGraph = function(parent, name, sharpness) {
  EventEmitter.decorate(this);

  this._parent = parent;
  this._ready = promise.defer();

  this._uid = "canvas-graph-" + Date.now();
  this._renderTargets = new Map();

  AbstractCanvasGraph.createIframe(GRAPH_SRC, parent, iframe => {
    this._iframe = iframe;
    this._window = iframe.contentWindow;
    this._topWindow = this._window.top;
    this._document = iframe.contentDocument;
    this._pixelRatio = sharpness || this._window.devicePixelRatio;

    let container = this._container = this._document.getElementById("graph-container");
    container.className = name + "-widget-container graph-widget-container";

    let canvas = this._canvas = this._document.getElementById("graph-canvas");
    canvas.className = name + "-widget-canvas graph-widget-canvas";

    let bounds = parent.getBoundingClientRect();
    bounds.width = this.fixedWidth || bounds.width;
    bounds.height = this.fixedHeight || bounds.height;
    iframe.setAttribute("width", bounds.width);
    iframe.setAttribute("height", bounds.height);

    this._width = canvas.width = bounds.width * this._pixelRatio;
    this._height = canvas.height = bounds.height * this._pixelRatio;
    this._ctx = canvas.getContext("2d");
    this._ctx.mozImageSmoothingEnabled = false;

    this._cursor = new GraphCursor();
    this._selection = new GraphArea();
    this._selectionDragger = new GraphAreaDragger();
    this._selectionResizer = new GraphAreaResizer();
    this._isMouseActive = false;

    this._onAnimationFrame = this._onAnimationFrame.bind(this);
    this._onMouseMove = this._onMouseMove.bind(this);
    this._onMouseDown = this._onMouseDown.bind(this);
    this._onMouseUp = this._onMouseUp.bind(this);
    this._onMouseWheel = this._onMouseWheel.bind(this);
    this._onMouseOut = this._onMouseOut.bind(this);
    this._onResize = this._onResize.bind(this);
    this.refresh = this.refresh.bind(this);

    this._window.addEventListener("mousemove", this._onMouseMove);
    this._window.addEventListener("mousedown", this._onMouseDown);
    this._window.addEventListener("MozMousePixelScroll", this._onMouseWheel);
    this._window.addEventListener("mouseout", this._onMouseOut);

    let ownerWindow = this._parent.ownerDocument.defaultView;
    ownerWindow.addEventListener("resize", this._onResize);

    this._animationId = this._window.requestAnimationFrame(this._onAnimationFrame);

    this._ready.resolve(this);
    this.emit("ready", this);
  });
};

AbstractCanvasGraph.prototype = {
  



  get width() {
    return this._width;
  },
  get height() {
    return this._height;
  },

  



  get isMouseActive() {
    return this._isMouseActive;
  },

  


  ready: function() {
    return this._ready.promise;
  },

  


  destroy: Task.async(function *() {
    yield this.ready();

    this._topWindow.removeEventListener("mousemove", this._onMouseMove);
    this._topWindow.removeEventListener("mouseup", this._onMouseUp);
    this._window.removeEventListener("mousemove", this._onMouseMove);
    this._window.removeEventListener("mousedown", this._onMouseDown);
    this._window.removeEventListener("MozMousePixelScroll", this._onMouseWheel);
    this._window.removeEventListener("mouseout", this._onMouseOut);

    let ownerWindow = this._parent.ownerDocument.defaultView;
    if (ownerWindow) {
      ownerWindow.removeEventListener("resize", this._onResize);
    }

    this._window.cancelAnimationFrame(this._animationId);
    this._iframe.remove();

    this._cursor = null;
    this._selection = null;
    this._selectionDragger = null;
    this._selectionResizer = null;

    this._data = null;
    this._mask = null;
    this._maskArgs = null;
    this._regions = null;

    this._cachedBackgroundImage = null;
    this._cachedGraphImage = null;
    this._cachedMaskImage = null;
    this._renderTargets.clear();
    gCachedStripePattern.clear();

    this.emit("destroyed");
  }),

  


  clipheadLineWidth: 1,
  clipheadLineColor: "transparent",
  selectionLineWidth: 1,
  selectionLineColor: "transparent",
  selectionBackgroundColor: "transparent",
  selectionStripesColor: "transparent",
  regionBackgroundColor: "transparent",
  regionStripesColor: "transparent",

  



  fixedWidth: null,
  fixedHeight: null,

  



  buildBackgroundImage: function() {
    return null;
  },

  




  buildGraphImage: function() {
    throw "This method needs to be implemented by inheriting classes.";
  },

  




  buildMaskImage: function() {
    return null;
  },

  



  dataScaleX: 1,
  dataScaleY: 1,

  





  setData: function(data) {
    this._data = data;
    this._cachedBackgroundImage = this.buildBackgroundImage();
    this._cachedGraphImage = this.buildGraphImage();
    this._shouldRedraw = true;
  },

  







  setDataWhenReady: Task.async(function*(data) {
    yield this.ready();
    this.setData(data);
  }),

  





  setMask: function(mask, ...options) {
    this._mask = mask;
    this._maskArgs = [mask, ...options];
    this._cachedMaskImage = this.buildMaskImage.apply(this, this._maskArgs);
    this._shouldRedraw = true;
  },

  








  setRegions: function(regions) {
    if (!this._cachedGraphImage) {
      throw "Can't highlight regions on a graph with no data displayed.";
    }
    if (this._regions) {
      throw "Regions were already highlighted on the graph.";
    }
    this._regions = regions.map(e => ({
      start: e.start * this.dataScaleX,
      end: e.end * this.dataScaleX
    }));
    this._bakeRegions(this._regions, this._cachedGraphImage);
    this._shouldRedraw = true;
  },

  



  hasData: function() {
    return !!this._data;
  },

  



  hasMask: function() {
    return !!this._mask;
  },

  



  hasRegions: function() {
    return !!this._regions;
  },

  











  setSelection: function(selection) {
    if (!selection || selection.start == null || selection.end == null) {
      throw "Invalid selection coordinates";
    }
    if (!this.isSelectionDifferent(selection)) {
      return;
    }
    this._selection.start = selection.start;
    this._selection.end = selection.end;
    this._shouldRedraw = true;
    this.emit("selecting");
  },

  






  getSelection: function() {
    if (this.hasSelection()) {
      return { start: this._selection.start, end: this._selection.end };
    }
    if (this.hasSelectionInProgress()) {
      return { start: this._selection.start, end: this._cursor.x };
    }
    return { start: null, end: null };
  },

  









  setMappedSelection: function(selection, mapping = {}) {
    if (!this.hasData()) {
      throw "A data source is necessary for retrieving a mapped selection.";
    }
    if (!selection || selection.start == null || selection.end == null) {
      throw "Invalid selection coordinates";
    }

    let { mapStart, mapEnd } = mapping;
    let startTime = (mapStart || (e => e.delta))(this._data[0]);
    let endTime = (mapEnd || (e => e.delta))(this._data[this._data.length - 1]);

    
    
    let min = Math.max(Math.min(selection.start, selection.end), startTime);
    let max = Math.min(Math.max(selection.start, selection.end), endTime);
    min = map(min, startTime, endTime, 0, this._width);
    max = map(max, startTime, endTime, 0, this._width);

    this.setSelection({ start: min, end: max });
  },

  









  getMappedSelection: function(mapping = {}) {
    if (!this.hasData()) {
      throw "A data source is necessary for retrieving a mapped selection.";
    }
    if (!this.hasSelection() && !this.hasSelectionInProgress()) {
      return { min: null, max: null };
    }

    let { mapStart, mapEnd } = mapping;
    let startTime = (mapStart || (e => e.delta))(this._data[0]);
    let endTime = (mapEnd || (e => e.delta))(this._data[this._data.length - 1]);

    
    
    
    let selection = this.getSelection();
    let min = Math.max(Math.min(selection.start, selection.end), 0);
    let max = Math.min(Math.max(selection.start, selection.end), this._width);
    min = map(min, 0, this._width, startTime, endTime);
    max = map(max, 0, this._width, startTime, endTime);

    return { min: min, max: max };
  },

  


  dropSelection: function() {
    if (!this.hasSelection() && !this.hasSelectionInProgress()) {
      return;
    }
    this._selection.start = null;
    this._selection.end = null;
    this._shouldRedraw = true;
    this.emit("deselecting");
  },

  



  hasSelection: function() {
    return this._selection &&
      this._selection.start != null && this._selection.end != null;
  },

  




  hasSelectionInProgress: function() {
    return this._selection &&
      this._selection.start != null && this._selection.end == null;
  },

  



  selectionEnabled: true,

  






  setCursor: function(cursor) {
    if (!cursor || cursor.x == null || cursor.y == null) {
      throw "Invalid cursor coordinates";
    }
    if (!this.isCursorDifferent(cursor)) {
      return;
    }
    this._cursor.x = cursor.x;
    this._cursor.y = cursor.y;
    this._shouldRedraw = true;
  },

  






  getCursor: function() {
    return { x: this._cursor.x, y: this._cursor.y };
  },

  


  dropCursor: function() {
    if (!this.hasCursor()) {
      return;
    }
    this._cursor.x = null;
    this._cursor.y = null;
    this._shouldRedraw = true;
  },

  



  hasCursor: function() {
    return this._cursor && this._cursor.x != null;
  },

  





  isSelectionDifferent: function(other) {
    if (!other) return true;
    let current = this.getSelection();
    return current.start != other.start || current.end != other.end;
  },

  





  isCursorDifferent: function(other) {
    if (!other) return true;
    let current = this.getCursor();
    return current.x != other.x || current.y != other.y;
  },

  






  getSelectionWidth: function() {
    let selection = this.getSelection();
    return Math.abs(selection.start - selection.end);
  },

  






  getHoveredRegion: function() {
    if (!this.hasRegions() || !this.hasCursor()) {
      return null;
    }
    let { x } = this._cursor;
    return this._regions.find(({ start, end }) =>
      (start < end && start < x && end > x) ||
      (start > end && end < x && start > x));
  },

  





  refresh: function(options={}) {
    let bounds = this._parent.getBoundingClientRect();
    let newWidth = this.fixedWidth || bounds.width;
    let newHeight = this.fixedHeight || bounds.height;

    
    
    if (!options.force &&
        this._width == newWidth * this._pixelRatio &&
        this._height == newHeight * this._pixelRatio) {
      this.emit("refresh-cancelled");
      return;
    }

    
    if (this._width && newWidth && this.hasSelection()) {
      let ratio = this._width / (newWidth * this._pixelRatio);
      this._selection.start = Math.round(this._selection.start / ratio);
      this._selection.end = Math.round(this._selection.end / ratio);
    }

    bounds.width = newWidth;
    bounds.height = newHeight;
    this._iframe.setAttribute("width", bounds.width);
    this._iframe.setAttribute("height", bounds.height);
    this._width = this._canvas.width = bounds.width * this._pixelRatio;
    this._height = this._canvas.height = bounds.height * this._pixelRatio;

    if (this.hasData()) {
      this._cachedBackgroundImage = this.buildBackgroundImage();
      this._cachedGraphImage = this.buildGraphImage();
    }
    if (this.hasMask()) {
      this._cachedMaskImage = this.buildMaskImage.apply(this, this._maskArgs);
    }
    if (this.hasRegions()) {
      this._bakeRegions(this._regions, this._cachedGraphImage);
    }

    this._shouldRedraw = true;
    this.emit("refresh");
  },

  











  _getNamedCanvas: function(name, width = this._width, height = this._height) {
    let cachedRenderTarget = this._renderTargets.get(name);
    if (cachedRenderTarget) {
      let { canvas, ctx } = cachedRenderTarget;
      canvas.width = width;
      canvas.height = height;
      ctx.clearRect(0, 0, width, height);
      return cachedRenderTarget;
    }

    let canvas = this._document.createElementNS(HTML_NS, "canvas");
    let ctx = canvas.getContext("2d");
    canvas.width = width;
    canvas.height = height;

    let renderTarget = { canvas: canvas, ctx: ctx };
    this._renderTargets.set(name, renderTarget);
    return renderTarget;
  },

  




  _shouldRedraw: false,

  


  _onAnimationFrame: function() {
    this._animationId = this._window.requestAnimationFrame(this._onAnimationFrame);
    this._drawWidget();
  },

  



  _drawWidget: function() {
    if (!this._shouldRedraw) {
      return;
    }
    let ctx = this._ctx;
    ctx.clearRect(0, 0, this._width, this._height);

    if (this._cachedGraphImage) {
      ctx.drawImage(this._cachedGraphImage, 0, 0, this._width, this._height);
    }
    if (this._cachedMaskImage) {
      ctx.globalCompositeOperation = "destination-out";
      ctx.drawImage(this._cachedMaskImage, 0, 0, this._width, this._height);
    }
    if (this._cachedBackgroundImage) {
      ctx.globalCompositeOperation = "destination-over";
      ctx.drawImage(this._cachedBackgroundImage, 0, 0, this._width, this._height);
    }

    
    if (this._cachedMaskImage || this._cachedBackgroundImage) {
      ctx.globalCompositeOperation = "source-over";
    }

    if (this.hasCursor()) {
      this._drawCliphead();
    }
    if (this.hasSelection() || this.hasSelectionInProgress()) {
      this._drawSelection();
    }

    this._shouldRedraw = false;
  },

  


  _drawCliphead: function() {
    if (this._isHoveringSelectionContentsOrBoundaries() || this._isHoveringRegion()) {
      return;
    }

    let ctx = this._ctx;
    ctx.lineWidth = this.clipheadLineWidth;
    ctx.strokeStyle = this.clipheadLineColor;
    ctx.beginPath();
    ctx.moveTo(this._cursor.x, 0);
    ctx.lineTo(this._cursor.x, this._height);
    ctx.stroke();
  },

  


  _drawSelection: function() {
    let { start, end } = this.getSelection();
    let input = this._canvas.getAttribute("input");

    let ctx = this._ctx;
    ctx.strokeStyle = this.selectionLineColor;

    

    let pattern = AbstractCanvasGraph.getStripePattern({
      ownerDocument: this._document,
      backgroundColor: this.selectionBackgroundColor,
      stripesColor: this.selectionStripesColor
    });
    ctx.fillStyle = pattern;
    let rectStart = Math.min(this._width, Math.max(0, start));
    let rectEnd = Math.min(this._width, Math.max(0, end));
    ctx.fillRect(rectStart, 0, rectEnd - rectStart, this._height);

    

    if (input == "hovering-selection-start-boundary") {
      ctx.lineWidth = GRAPH_SELECTION_BOUNDARY_HOVER_LINE_WIDTH;
    } else {
      ctx.lineWidth = this.clipheadLineWidth;
    }
    ctx.beginPath();
    ctx.moveTo(start, 0);
    ctx.lineTo(start, this._height);
    ctx.stroke();

    

    if (input == "hovering-selection-end-boundary") {
      ctx.lineWidth = GRAPH_SELECTION_BOUNDARY_HOVER_LINE_WIDTH;
    } else {
      ctx.lineWidth = this.clipheadLineWidth;
    }
    ctx.beginPath();
    ctx.moveTo(end, this._height);
    ctx.lineTo(end, 0);
    ctx.stroke();
  },

  



  _bakeRegions: function(regions, destination) {
    let ctx = destination.getContext("2d");

    let pattern = AbstractCanvasGraph.getStripePattern({
      ownerDocument: this._document,
      backgroundColor: this.regionBackgroundColor,
      stripesColor: this.regionStripesColor
    });
    ctx.fillStyle = pattern;
    ctx.strokeStyle = GRAPH_REGION_LINE_COLOR;
    ctx.lineWidth = GRAPH_REGION_LINE_WIDTH;

    let y = -GRAPH_REGION_LINE_WIDTH;
    let height = this._height + GRAPH_REGION_LINE_WIDTH;

    for (let { start, end } of regions) {
      let x = start;
      let width = end - start;
      ctx.fillRect(x, y, width, height);
      ctx.strokeRect(x, y, width, height);
    }
  },

  



  _isHoveringStartBoundary: function() {
    if (!this.hasSelection() || !this.hasCursor()) {
      return;
    }
    let { x } = this._cursor;
    let { start } = this._selection;
    let threshold = GRAPH_SELECTION_BOUNDARY_HOVER_THRESHOLD * this._pixelRatio;
    return Math.abs(start - x) < threshold;
  },

  



  _isHoveringEndBoundary: function() {
    if (!this.hasSelection() || !this.hasCursor()) {
      return;
    }
    let { x } = this._cursor;
    let { end } = this._selection;
    let threshold = GRAPH_SELECTION_BOUNDARY_HOVER_THRESHOLD * this._pixelRatio;
    return Math.abs(end - x) < threshold;
  },

  



  _isHoveringSelectionContents: function() {
    if (!this.hasSelection() || !this.hasCursor()) {
      return;
    }
    let { x } = this._cursor;
    let { start, end } = this._selection;
    return (start < end && start < x && end > x) ||
           (start > end && end < x && start > x);
  },

  



  _isHoveringSelectionContentsOrBoundaries: function() {
    return this._isHoveringSelectionContents() ||
           this._isHoveringStartBoundary() ||
           this._isHoveringEndBoundary();
  },

  



  _isHoveringRegion: function() {
    return !!this.getHoveredRegion();
  },

  



  _getRelativeEventCoordinates: function(e) {
    
    
    if ("testX" in e && "testY" in e) {
      return {
        mouseX: e.testX * this._pixelRatio,
        mouseY: e.testY * this._pixelRatio
      };
    }

    let quad = this._canvas.getBoxQuads({
      relativeTo: this._topWindow.document
    })[0];

    let x = (e.screenX - this._topWindow.screenX) - quad.p1.x;
    let y = (e.screenY - this._topWindow.screenY) - quad.p1.y;

    
    
    let maxX = quad.p2.x - quad.p1.x;
    let maxY = quad.p3.y - quad.p1.y;
    let mouseX = Math.max(0, Math.min(x, maxX)) * this._pixelRatio;
    let mouseY = Math.max(0, Math.min(x, maxY)) * this._pixelRatio;

    
    
    let zoom = LayoutHelpers.getCurrentZoom(this._canvas);
    mouseX /= zoom;
    mouseY /= zoom;

    return {mouseX,mouseY};
  },

  


  _onMouseMove: function(e) {
    let resizer = this._selectionResizer;
    let dragger = this._selectionDragger;

    
    
    
    
    if (e.stopPropagation && this._isMouseActive) {
      e.stopPropagation();
    }

    
    
    if (e.buttons == 0 && (this.hasSelectionInProgress() ||
                           resizer.margin != null ||
                           dragger.origin != null)) {
      return this._onMouseUp();
    }

    let {mouseX,mouseY} = this._getRelativeEventCoordinates(e);
    this._cursor.x = mouseX;
    this._cursor.y = mouseY;

    if (resizer.margin != null) {
      this._selection[resizer.margin] = mouseX;
      this._shouldRedraw = true;
      this.emit("selecting");
      return;
    }

    if (dragger.origin != null) {
      this._selection.start = dragger.anchor.start - dragger.origin + mouseX;
      this._selection.end = dragger.anchor.end - dragger.origin + mouseX;
      this._shouldRedraw = true;
      this.emit("selecting");
      return;
    }

    if (this.hasSelectionInProgress()) {
      this._shouldRedraw = true;
      this.emit("selecting");
      return;
    }

    if (this.hasSelection()) {
      if (this._isHoveringStartBoundary()) {
        this._canvas.setAttribute("input", "hovering-selection-start-boundary");
        this._shouldRedraw = true;
        return;
      }
      if (this._isHoveringEndBoundary()) {
        this._canvas.setAttribute("input", "hovering-selection-end-boundary");
        this._shouldRedraw = true;
        return;
      }
      if (this._isHoveringSelectionContents()) {
        this._canvas.setAttribute("input", "hovering-selection-contents");
        this._shouldRedraw = true;
        return;
      }
    }

    let region = this.getHoveredRegion();
    if (region) {
      this._canvas.setAttribute("input", "hovering-region");
    } else {
      this._canvas.setAttribute("input", "hovering-background");
    }

    this._shouldRedraw = true;
  },

  


  _onMouseDown: function(e) {
    this._isMouseActive = true;
    let {mouseX} = this._getRelativeEventCoordinates(e);

    switch (this._canvas.getAttribute("input")) {
      case "hovering-background":
      case "hovering-region":
        if (!this.selectionEnabled) {
          break;
        }
        this._selection.start = mouseX;
        this._selection.end = null;
        this.emit("selecting");
        break;

      case "hovering-selection-start-boundary":
        this._selectionResizer.margin = "start";
        break;

      case "hovering-selection-end-boundary":
        this._selectionResizer.margin = "end";
        break;

      case "hovering-selection-contents":
        this._selectionDragger.origin = mouseX;
        this._selectionDragger.anchor.start = this._selection.start;
        this._selectionDragger.anchor.end = this._selection.end;
        this._canvas.setAttribute("input", "dragging-selection-contents");
        break;
    }

    
    
    this._topWindow.addEventListener("mousemove", this._onMouseMove);
    this._topWindow.addEventListener("mouseup", this._onMouseUp);

    this._shouldRedraw = true;
    this.emit("mousedown");
  },

  


  _onMouseUp: function() {
    this._isMouseActive = false;
    switch (this._canvas.getAttribute("input")) {
      case "hovering-background":
      case "hovering-region":
        if (!this.selectionEnabled) {
          break;
        }
        if (this.getSelectionWidth() < 1) {
          let region = this.getHoveredRegion();
          if (region) {
            this._selection.start = region.start;
            this._selection.end = region.end;
            this.emit("selecting");
          } else {
            this._selection.start = null;
            this._selection.end = null;
            this.emit("deselecting");
          }
        } else {
          this._selection.end = this._cursor.x;
          this.emit("selecting");
        }
        break;

      case "hovering-selection-start-boundary":
      case "hovering-selection-end-boundary":
        this._selectionResizer.margin = null;
        break;

      case "dragging-selection-contents":
        this._selectionDragger.origin = null;
        this._canvas.setAttribute("input", "hovering-selection-contents");
        break;
    }

    
    this._topWindow.removeEventListener("mousemove", this._onMouseMove);
    this._topWindow.removeEventListener("mouseup", this._onMouseUp);

    this._shouldRedraw = true;
    this.emit("mouseup");
  },

  


  _onMouseWheel: function(e) {
    if (!this.hasSelection()) {
      return;
    }

    let {mouseX} = this._getRelativeEventCoordinates(e);
    let focusX = mouseX;

    let selection = this._selection;
    let vector = 0;

    
    
    if (this._isHoveringSelectionContentsOrBoundaries()) {
      let distStart = selection.start - focusX;
      let distEnd = selection.end - focusX;
      vector = e.detail * GRAPH_WHEEL_ZOOM_SENSITIVITY;
      selection.start = selection.start + distStart * vector;
      selection.end = selection.end + distEnd * vector;
    }
    
    else {
      let direction = 0;
      if (focusX > selection.end) {
        direction = Math.sign(focusX - selection.end);
      } else if (focusX < selection.start) {
        direction = Math.sign(focusX - selection.start);
      }
      vector = direction * e.detail * GRAPH_WHEEL_SCROLL_SENSITIVITY;
      selection.start -= vector;
      selection.end -= vector;
    }

    
    

    let minStart = GRAPH_MAX_SELECTION_LEFT_PADDING;
    let maxEnd = this._width - GRAPH_MAX_SELECTION_RIGHT_PADDING;
    if (selection.start < minStart) {
      selection.start = minStart;
    }
    if (selection.start > maxEnd) {
      selection.start = maxEnd;
    }
    if (selection.end < minStart) {
      selection.end = minStart;
    }
    if (selection.end > maxEnd) {
      selection.end = maxEnd;
    }

    

    let thickness = Math.abs(selection.start - selection.end);
    if (thickness < GRAPH_WHEEL_MIN_SELECTION_WIDTH) {
      let midPoint = (selection.start + selection.end) / 2;
      selection.start = midPoint - GRAPH_WHEEL_MIN_SELECTION_WIDTH / 2;
      selection.end = midPoint + GRAPH_WHEEL_MIN_SELECTION_WIDTH / 2;
    }

    this._shouldRedraw = true;
    this.emit("selecting");
    this.emit("scroll");
  },

  



  _onMouseOut: function(e) {
    if (!this._isMouseActive) {
      this._cursor.x = null;
      this._cursor.y = null;
      this._canvas.removeAttribute("input");
      this._shouldRedraw = true;
    }
  },

  


  _onResize: function() {
    if (this.hasData()) {
      setNamedTimeout(this._uid, GRAPH_RESIZE_EVENTS_DRAIN, this.refresh);
    }
  }
};














AbstractCanvasGraph.createIframe = function(url, parent, callback) {
  let iframe = parent.ownerDocument.createElementNS(HTML_NS, "iframe");

  iframe.addEventListener("DOMContentLoaded", function onLoad() {
    iframe.removeEventListener("DOMContentLoaded", onLoad);
    callback(iframe);
  });

  
  
  iframe.setAttribute("frameborder", "0");
  iframe.style.width = "100%";
  iframe.style.minWidth = "50px";
  iframe.src = url;

  parent.style.display = "flex";
  parent.appendChild(iframe);
};












AbstractCanvasGraph.getStripePattern = function(data) {
  let { ownerDocument, backgroundColor, stripesColor } = data;
  let id = [backgroundColor, stripesColor].join(",");

  if (gCachedStripePattern.has(id)) {
    return gCachedStripePattern.get(id);
  }

  let canvas = ownerDocument.createElementNS(HTML_NS, "canvas");
  let ctx = canvas.getContext("2d");
  let width = canvas.width = GRAPH_STRIPE_PATTERN_WIDTH;
  let height = canvas.height = GRAPH_STRIPE_PATTERN_HEIGHT;

  ctx.fillStyle = backgroundColor;
  ctx.fillRect(0, 0, width, height);

  let pixelRatio = ownerDocument.defaultView.devicePixelRatio;
  let scaledLineWidth = GRAPH_STRIPE_PATTERN_LINE_WIDTH * pixelRatio;
  let scaledLineSpacing = GRAPH_STRIPE_PATTERN_LINE_SPACING * pixelRatio;

  ctx.strokeStyle = stripesColor;
  ctx.lineWidth = scaledLineWidth;
  ctx.lineCap = "square";
  ctx.beginPath();

  for (let i = -height; i <= height; i += scaledLineSpacing) {
    ctx.moveTo(width, i);
    ctx.lineTo(0, i + height);
  }

  ctx.stroke();

  let pattern = ctx.createPattern(canvas, "repeat");
  gCachedStripePattern.set(id, pattern);
  return pattern;
};




const gCachedStripePattern = new Map();




this.CanvasGraphUtils = {
  _graphUtilsWorker: null,
  _graphUtilsTaskId: 0,

  


  linkAnimation: Task.async(function*(graph1, graph2) {
    if (!graph1 || !graph2) {
      return;
    }
    yield graph1.ready();
    yield graph2.ready();

    let window = graph1._window;
    window.cancelAnimationFrame(graph1._animationId);
    window.cancelAnimationFrame(graph2._animationId);

    let loop = () => {
      window.requestAnimationFrame(loop);
      graph1._drawWidget();
      graph2._drawWidget();
    };

    window.requestAnimationFrame(loop);
  }),

  


  linkSelection: function(graph1, graph2) {
    if (!graph1 || !graph2) {
      return;
    }

    if (graph1.hasSelection()) {
      graph2.setSelection(graph1.getSelection());
    } else {
      graph2.dropSelection();
    }

    graph1.on("selecting", () => {
      graph2.setSelection(graph1.getSelection());
    });
    graph2.on("selecting", () => {
      graph1.setSelection(graph2.getSelection());
    });
    graph1.on("deselecting", () => {
      graph2.dropSelection();
    });
    graph2.on("deselecting", () => {
      graph1.dropSelection();
    });
  },

  









  _performTaskInWorker: function(task, data) {
    let worker = this._graphUtilsWorker || new DevToolsWorker(WORKER_URL);
    return worker.performTask(task, data);
  }
};






function map(value, istart, istop, ostart, ostop) {
  let ratio = istop - istart;
  if (ratio == 0) {
    return value;
  }
  return ostart + (ostop - ostart) * ((value - istart) / ratio);
}






function clamp(value, min, max) {
  if (value < min) return min;
  if (value > max) return max;
  return value;
}

exports.GraphCursor = GraphCursor;
exports.GraphArea = GraphArea;
exports.GraphAreaDragger = GraphAreaDragger;
exports.GraphAreaResizer = GraphAreaResizer;
exports.AbstractCanvasGraph = AbstractCanvasGraph;
exports.CanvasGraphUtils = CanvasGraphUtils;
exports.CanvasGraphUtils.map = map;
exports.CanvasGraphUtils.clamp = clamp;
