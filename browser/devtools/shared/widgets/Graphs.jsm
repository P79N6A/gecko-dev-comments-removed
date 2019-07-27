


"use strict";

const Cu = Components.utils;

Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
const promise = Cu.import("resource://gre/modules/Promise.jsm", {}).Promise;
const {Task} = Cu.import("resource://gre/modules/Task.jsm", {});
const {EventEmitter} = Cu.import("resource://gre/modules/devtools/event-emitter.js", {});

this.EXPORTED_SYMBOLS = [
  "GraphCursor",
  "GraphArea",
  "GraphAreaDragger",
  "GraphAreaResizer",
  "AbstractCanvasGraph",
  "LineGraphWidget",
  "BarGraphWidget",
  "CanvasGraphUtils"
];

const HTML_NS = "http://www.w3.org/1999/xhtml";
const GRAPH_SRC = "chrome://browser/content/devtools/graphs-frame.xhtml";
const WORKER_URL = "resource:///modules/devtools/GraphsWorker.js";
const L10N = new ViewHelpers.L10N();



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



const LINE_GRAPH_DAMPEN_VALUES = 0.85;
const LINE_GRAPH_TOOLTIP_SAFE_BOUNDS = 8; 
const LINE_GRAPH_MIN_MAX_TOOLTIP_DISTANCE = 14; 

const LINE_GRAPH_BACKGROUND_COLOR = "#0088cc";
const LINE_GRAPH_STROKE_WIDTH = 1; 
const LINE_GRAPH_STROKE_COLOR = "rgba(255,255,255,0.9)";
const LINE_GRAPH_HELPER_LINES_DASH = [5]; 
const LINE_GRAPH_HELPER_LINES_WIDTH = 1; 
const LINE_GRAPH_MAXIMUM_LINE_COLOR = "rgba(255,255,255,0.4)";
const LINE_GRAPH_AVERAGE_LINE_COLOR = "rgba(255,255,255,0.7)";
const LINE_GRAPH_MINIMUM_LINE_COLOR = "rgba(255,255,255,0.9)";
const LINE_GRAPH_BACKGROUND_GRADIENT_START = "rgba(255,255,255,0.25)";
const LINE_GRAPH_BACKGROUND_GRADIENT_END = "rgba(255,255,255,0.0)";

const LINE_GRAPH_CLIPHEAD_LINE_COLOR = "#fff";
const LINE_GRAPH_SELECTION_LINE_COLOR = "#fff";
const LINE_GRAPH_SELECTION_BACKGROUND_COLOR = "rgba(44,187,15,0.25)";
const LINE_GRAPH_SELECTION_STRIPES_COLOR = "rgba(255,255,255,0.1)";
const LINE_GRAPH_REGION_BACKGROUND_COLOR = "transparent";
const LINE_GRAPH_REGION_STRIPES_COLOR = "rgba(237,38,85,0.2)";



const BAR_GRAPH_DAMPEN_VALUES = 0.75;
const BAR_GRAPH_BARS_MARGIN_TOP = 1; 
const BAR_GRAPH_BARS_MARGIN_END = 1; 
const BAR_GRAPH_MIN_BARS_WIDTH = 5; 
const BAR_GRAPH_MIN_BLOCKS_HEIGHT = 1; 

const BAR_GRAPH_BACKGROUND_GRADIENT_START = "rgba(0,136,204,0.0)";
const BAR_GRAPH_BACKGROUND_GRADIENT_END = "rgba(255,255,255,0.25)";

const BAR_GRAPH_CLIPHEAD_LINE_COLOR = "#666";
const BAR_GRAPH_SELECTION_LINE_COLOR = "#555";
const BAR_GRAPH_SELECTION_BACKGROUND_COLOR = "rgba(0,136,204,0.25)";
const BAR_GRAPH_SELECTION_STRIPES_COLOR = "rgba(255,255,255,0.1)";
const BAR_GRAPH_REGION_BACKGROUND_COLOR = "transparent";
const BAR_GRAPH_REGION_STRIPES_COLOR = "rgba(237,38,85,0.2)";

const BAR_GRAPH_HIGHLIGHTS_MASK_BACKGROUND = "rgba(255,255,255,0.75)";
const BAR_GRAPH_HIGHLIGHTS_MASK_STRIPES = "rgba(255,255,255,0.5)";

const BAR_GRAPH_LEGEND_MOUSEOVER_DEBOUNCE = 50; 




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
    this._window.addEventListener("mouseup", this._onMouseUp);
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

  


  ready: function() {
    return this._ready.promise;
  },

  


  destroy: Task.async(function *() {
    yield this.ready();

    this._window.removeEventListener("mousemove", this._onMouseMove);
    this._window.removeEventListener("mousedown", this._onMouseDown);
    this._window.removeEventListener("mouseup", this._onMouseUp);
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
    ctx.fillRect(start, 0, end - start, this._height);

    

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

  





  _getContainerOffset: function() {
    let node = this._canvas;
    let x = 0;
    let y = 0;

    while (node = node.offsetParent) {
      x += node.offsetLeft;
      y += node.offsetTop;
    }

    return { left: x, top: y };
  },

  


  _onMouseMove: function(e) {
    let resizer = this._selectionResizer;
    let dragger = this._selectionDragger;

    
    
    if (e.buttons == 0 && (this.hasSelectionInProgress() ||
                           resizer.margin != null ||
                           dragger.origin != null)) {
      return this._onMouseUp(e);
    }

    let offset = this._getContainerOffset();
    let mouseX = (e.clientX - offset.left) * this._pixelRatio;
    let mouseY = (e.clientY - offset.top) * this._pixelRatio;
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
    let offset = this._getContainerOffset();
    let mouseX = (e.clientX - offset.left) * this._pixelRatio;

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

    this._shouldRedraw = true;
    this.emit("mousedown");
  },

  


  _onMouseUp: function(e) {
    this._isMouseActive = false;
    let offset = this._getContainerOffset();
    let mouseX = (e.clientX - offset.left) * this._pixelRatio;

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
          this._selection.end = mouseX;
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

    this._shouldRedraw = true;
    this.emit("mouseup");
  },

  


  _onMouseWheel: function(e) {
    if (!this.hasSelection()) {
      return;
    }

    let offset = this._getContainerOffset();
    let mouseX = (e.clientX - offset.left) * this._pixelRatio;
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






























this.LineGraphWidget = function(parent, options, ...args) {
  options = options || {};
  let metric = options.metric;

  this._showMin = options.min !== false;
  this._showMax = options.max !== false;
  this._showAvg = options.avg !== false;
  AbstractCanvasGraph.apply(this, [parent, "line-graph", ...args]);

  this.once("ready", () => {
    
    
    this._gutter = this._createGutter();

    this._maxGutterLine = this._createGutterLine("maximum");
    this._maxTooltip = this._createTooltip("maximum", "start", "max", metric);
    this._minGutterLine = this._createGutterLine("minimum");
    this._minTooltip = this._createTooltip("minimum", "start", "min", metric);
    this._avgGutterLine = this._createGutterLine("average");
    this._avgTooltip = this._createTooltip("average", "end", "avg", metric);
  });
};

LineGraphWidget.prototype = Heritage.extend(AbstractCanvasGraph.prototype, {
  backgroundColor: LINE_GRAPH_BACKGROUND_COLOR,
  backgroundGradientStart: LINE_GRAPH_BACKGROUND_GRADIENT_START,
  backgroundGradientEnd: LINE_GRAPH_BACKGROUND_GRADIENT_END,
  strokeColor: LINE_GRAPH_STROKE_COLOR,
  strokeWidth: LINE_GRAPH_STROKE_WIDTH,
  maximumLineColor: LINE_GRAPH_MAXIMUM_LINE_COLOR,
  averageLineColor: LINE_GRAPH_AVERAGE_LINE_COLOR,
  minimumLineColor: LINE_GRAPH_MINIMUM_LINE_COLOR,
  clipheadLineColor: LINE_GRAPH_CLIPHEAD_LINE_COLOR,
  selectionLineColor: LINE_GRAPH_SELECTION_LINE_COLOR,
  selectionBackgroundColor: LINE_GRAPH_SELECTION_BACKGROUND_COLOR,
  selectionStripesColor: LINE_GRAPH_SELECTION_STRIPES_COLOR,
  regionBackgroundColor: LINE_GRAPH_REGION_BACKGROUND_COLOR,
  regionStripesColor: LINE_GRAPH_REGION_STRIPES_COLOR,

  


  dataOffsetX: 0,

  



  dataDuration: 0,

  


  dampenValuesFactor: LINE_GRAPH_DAMPEN_VALUES,

  


  withTooltipArrows: true,

  



  withFixedTooltipPositions: false,

  













  setDataFromTimestamps: Task.async(function*(timestamps, interval, duration) {
    let {
      plottedData,
      plottedMinMaxSum
    } = yield CanvasGraphUtils._performTaskInWorker("plotTimestampsGraph", {
      timestamps, interval, duration
    });

    this._tempMinMaxSum = plottedMinMaxSum;
    this.setData(plottedData);
  }),

  



  buildGraphImage: function() {
    let { canvas, ctx } = this._getNamedCanvas("line-graph-data");
    let width = this._width;
    let height = this._height;

    let totalTicks = this._data.length;
    let firstTick = totalTicks ? this._data[0].delta : 0;
    let lastTick = totalTicks ? this._data[totalTicks - 1].delta : 0;
    let maxValue = Number.MIN_SAFE_INTEGER;
    let minValue = Number.MAX_SAFE_INTEGER;
    let avgValue = 0;

    if (this._tempMinMaxSum) {
      maxValue = this._tempMinMaxSum.maxValue;
      minValue = this._tempMinMaxSum.minValue;
      avgValue = this._tempMinMaxSum.avgValue;
    } else {
      let sumValues = 0;
      for (let { delta, value } of this._data) {
        maxValue = Math.max(value, maxValue);
        minValue = Math.min(value, minValue);
        sumValues += value;
      }
      avgValue = sumValues / totalTicks;
    }

    let duration = this.dataDuration || lastTick;
    let dataScaleX = this.dataScaleX = width / (duration - this.dataOffsetX);
    let dataScaleY = this.dataScaleY = height / maxValue * this.dampenValuesFactor;

    

    ctx.fillStyle = this.backgroundColor;
    ctx.fillRect(0, 0, width, height);

    

    let gradient = ctx.createLinearGradient(0, height / 2, 0, height);
    gradient.addColorStop(0, this.backgroundGradientStart);
    gradient.addColorStop(1, this.backgroundGradientEnd);
    ctx.fillStyle = gradient;
    ctx.strokeStyle = this.strokeColor;
    ctx.lineWidth = this.strokeWidth * this._pixelRatio;
    ctx.beginPath();

    for (let { delta, value } of this._data) {
      let currX = (delta - this.dataOffsetX) * dataScaleX;
      let currY = height - value * dataScaleY;

      if (delta == firstTick) {
        ctx.moveTo(-LINE_GRAPH_STROKE_WIDTH, height);
        ctx.lineTo(-LINE_GRAPH_STROKE_WIDTH, currY);
      }

      ctx.lineTo(currX, currY);

      if (delta == lastTick) {
        ctx.lineTo(width + LINE_GRAPH_STROKE_WIDTH, currY);
        ctx.lineTo(width + LINE_GRAPH_STROKE_WIDTH, height);
      }
    }

    ctx.fill();
    ctx.stroke();

    this._drawOverlays(ctx, minValue, maxValue, avgValue, dataScaleY);

    return canvas;
  },

  









  _drawOverlays: function(ctx, minValue, maxValue, avgValue, dataScaleY) {
    let width = this._width;
    let height = this._height;
    let totalTicks = this._data.length;

    
    if (this._showMax) {
      ctx.strokeStyle = this.maximumLineColor;
      ctx.lineWidth = LINE_GRAPH_HELPER_LINES_WIDTH;
      ctx.setLineDash(LINE_GRAPH_HELPER_LINES_DASH);
      ctx.beginPath();
      let maximumY = height - maxValue * dataScaleY;
      ctx.moveTo(0, maximumY);
      ctx.lineTo(width, maximumY);
      ctx.stroke();
    }

    
    if (this._showAvg) {
      ctx.strokeStyle = this.averageLineColor;
      ctx.lineWidth = LINE_GRAPH_HELPER_LINES_WIDTH;
      ctx.setLineDash(LINE_GRAPH_HELPER_LINES_DASH);
      ctx.beginPath();
      let averageY = height - avgValue * dataScaleY;
      ctx.moveTo(0, averageY);
      ctx.lineTo(width, averageY);
      ctx.stroke();
    }

    
    if (this._showMin) {
      ctx.strokeStyle = this.minimumLineColor;
      ctx.lineWidth = LINE_GRAPH_HELPER_LINES_WIDTH;
      ctx.setLineDash(LINE_GRAPH_HELPER_LINES_DASH);
      ctx.beginPath();
      let minimumY = height - minValue * dataScaleY;
      ctx.moveTo(0, minimumY);
      ctx.lineTo(width, minimumY);
      ctx.stroke();
    }

    

    this._maxTooltip.querySelector("[text=value]").textContent =
      L10N.numberWithDecimals(maxValue, 2);
    this._avgTooltip.querySelector("[text=value]").textContent =
      L10N.numberWithDecimals(avgValue, 2);
    this._minTooltip.querySelector("[text=value]").textContent =
      L10N.numberWithDecimals(minValue, 2);

    let bottom = height / this._pixelRatio;
    let maxPosY = map(maxValue * this.dampenValuesFactor, 0, maxValue, bottom, 0);
    let avgPosY = map(avgValue * this.dampenValuesFactor, 0, maxValue, bottom, 0);
    let minPosY = map(minValue * this.dampenValuesFactor, 0, maxValue, bottom, 0);

    let safeTop = LINE_GRAPH_TOOLTIP_SAFE_BOUNDS;
    let safeBottom = bottom - LINE_GRAPH_TOOLTIP_SAFE_BOUNDS;

    let maxTooltipTop = (this.withFixedTooltipPositions
      ? safeTop : clamp(maxPosY, safeTop, safeBottom));
    let avgTooltipTop = (this.withFixedTooltipPositions
      ? safeTop : clamp(avgPosY, safeTop, safeBottom));
    let minTooltipTop = (this.withFixedTooltipPositions
      ? safeBottom : clamp(minPosY, safeTop, safeBottom));

    this._maxTooltip.style.top = maxTooltipTop + "px";
    this._avgTooltip.style.top = avgTooltipTop + "px";
    this._minTooltip.style.top = minTooltipTop + "px";

    this._maxGutterLine.style.top = maxPosY + "px";
    this._avgGutterLine.style.top = avgPosY + "px";
    this._minGutterLine.style.top = minPosY + "px";

    this._maxTooltip.setAttribute("with-arrows", this.withTooltipArrows);
    this._avgTooltip.setAttribute("with-arrows", this.withTooltipArrows);
    this._minTooltip.setAttribute("with-arrows", this.withTooltipArrows);

    let distanceMinMax = Math.abs(maxTooltipTop - minTooltipTop);
    this._maxTooltip.hidden = this._showMax === false || !totalTicks || distanceMinMax < LINE_GRAPH_MIN_MAX_TOOLTIP_DISTANCE;
    this._avgTooltip.hidden = this._showAvg === false || !totalTicks;
    this._minTooltip.hidden = this._showMin === false || !totalTicks;
    this._gutter.hidden = (this._showMin === false && this._showAvg === false && this._showMax === false) || !totalTicks;

    this._maxGutterLine.hidden = this._showMax === false;
    this._avgGutterLine.hidden = this._showAvg === false;
    this._minGutterLine.hidden = this._showMin === false;
  },

  



  _createGutter: function() {
    let gutter = this._document.createElementNS(HTML_NS, "div");
    gutter.className = "line-graph-widget-gutter";
    gutter.setAttribute("hidden", true);
    this._container.appendChild(gutter);

    return gutter;
  },

  



  _createGutterLine: function(type) {
    let line = this._document.createElementNS(HTML_NS, "div");
    line.className = "line-graph-widget-gutter-line";
    line.setAttribute("type", type);
    this._gutter.appendChild(line);

    return line;
  },

  



  _createTooltip: function(type, arrow, info, metric) {
    let tooltip = this._document.createElementNS(HTML_NS, "div");
    tooltip.className = "line-graph-widget-tooltip";
    tooltip.setAttribute("type", type);
    tooltip.setAttribute("arrow", arrow);
    tooltip.setAttribute("hidden", true);

    let infoNode = this._document.createElementNS(HTML_NS, "span");
    infoNode.textContent = info;
    infoNode.setAttribute("text", "info");

    let valueNode = this._document.createElementNS(HTML_NS, "span");
    valueNode.textContent = 0;
    valueNode.setAttribute("text", "value");

    let metricNode = this._document.createElementNS(HTML_NS, "span");
    metricNode.textContent = metric;
    metricNode.setAttribute("text", "metric");

    tooltip.appendChild(infoNode);
    tooltip.appendChild(valueNode);
    tooltip.appendChild(metricNode);
    this._container.appendChild(tooltip);

    return tooltip;
  }
});



































this.BarGraphWidget = function(parent, ...args) {
  AbstractCanvasGraph.apply(this, [parent, "bar-graph", ...args]);

  
  
  this.outstandingEventListeners = [];

  this.once("ready", () => {
    this._onLegendMouseOver = this._onLegendMouseOver.bind(this);
    this._onLegendMouseOut = this._onLegendMouseOut.bind(this);
    this._onLegendMouseDown = this._onLegendMouseDown.bind(this);
    this._onLegendMouseUp = this._onLegendMouseUp.bind(this);
    this._createLegend();
  });

  this.once("destroyed", () => {
    for (let [node, event, listener] of this.outstandingEventListeners) {
      node.removeEventListener(event, listener);
    }
    this.outstandingEventListeners = null;
  });
};

BarGraphWidget.prototype = Heritage.extend(AbstractCanvasGraph.prototype, {
  clipheadLineColor: BAR_GRAPH_CLIPHEAD_LINE_COLOR,
  selectionLineColor: BAR_GRAPH_SELECTION_LINE_COLOR,
  selectionBackgroundColor: BAR_GRAPH_SELECTION_BACKGROUND_COLOR,
  selectionStripesColor: BAR_GRAPH_SELECTION_STRIPES_COLOR,
  regionBackgroundColor: BAR_GRAPH_REGION_BACKGROUND_COLOR,
  regionStripesColor: BAR_GRAPH_REGION_STRIPES_COLOR,

  



  format: null,

  


  dataOffsetX: 0,

  



  dampenValuesFactor: BAR_GRAPH_DAMPEN_VALUES,

  



  minBarsWidth: BAR_GRAPH_MIN_BARS_WIDTH,

  



  minBlocksHeight: BAR_GRAPH_MIN_BLOCKS_HEIGHT,

  



  buildBackgroundImage: function() {
    let { canvas, ctx } = this._getNamedCanvas("bar-graph-background");
    let width = this._width;
    let height = this._height;

    let gradient = ctx.createLinearGradient(0, 0, 0, height);
    gradient.addColorStop(0, BAR_GRAPH_BACKGROUND_GRADIENT_START);
    gradient.addColorStop(1, BAR_GRAPH_BACKGROUND_GRADIENT_END);
    ctx.fillStyle = gradient;
    ctx.fillRect(0, 0, width, height);

    return canvas;
  },

  



  buildGraphImage: function() {
    if (!this.format || !this.format.length) {
      throw "The graph format traits are mandatory to style the data source.";
    }
    let { canvas, ctx } = this._getNamedCanvas("bar-graph-data");
    let width = this._width;
    let height = this._height;

    let totalTypes = this.format.length;
    let totalTicks = this._data.length;
    let lastTick = this._data[totalTicks - 1].delta;

    let minBarsWidth = this.minBarsWidth * this._pixelRatio;
    let minBlocksHeight = this.minBlocksHeight * this._pixelRatio;

    let dataScaleX = this.dataScaleX = width / (lastTick - this.dataOffsetX);
    let dataScaleY = this.dataScaleY = height / this._calcMaxHeight({
      data: this._data,
      dataScaleX: dataScaleX,
      minBarsWidth: minBarsWidth
    }) * this.dampenValuesFactor;

    

    
    
    

    this._blocksBoundingRects = [];
    let prevHeight = [];
    let scaledMarginEnd = BAR_GRAPH_BARS_MARGIN_END * this._pixelRatio;
    let unscaledMarginTop = BAR_GRAPH_BARS_MARGIN_TOP;

    for (let type = 0; type < totalTypes; type++) {
      ctx.fillStyle = this.format[type].color || "#000";
      ctx.beginPath();

      let prevRight = 0;
      let skippedCount = 0;
      let skippedHeight = 0;

      for (let tick = 0; tick < totalTicks; tick++) {
        let delta = this._data[tick].delta;
        let value = this._data[tick].values[type] || 0;
        let blockRight = (delta - this.dataOffsetX) * dataScaleX;
        let blockHeight = value * dataScaleY;

        let blockWidth = blockRight - prevRight;
        if (blockWidth < minBarsWidth) {
          skippedCount++;
          skippedHeight += blockHeight;
          continue;
        }

        let averageHeight = (blockHeight + skippedHeight) / (skippedCount + 1);
        if (averageHeight >= minBlocksHeight) {
          let bottom = height - ~~prevHeight[tick];
          ctx.moveTo(prevRight, bottom);
          ctx.lineTo(prevRight, bottom - averageHeight);
          ctx.lineTo(blockRight, bottom - averageHeight);
          ctx.lineTo(blockRight, bottom);

          
          this._blocksBoundingRects.push({
            type: type,
            start: prevRight,
            end: blockRight,
            top: bottom - averageHeight,
            bottom: bottom
          });

          if (prevHeight[tick] === undefined) {
            prevHeight[tick] = averageHeight + unscaledMarginTop;
          } else {
            prevHeight[tick] += averageHeight + unscaledMarginTop;
          }
        }

        prevRight += blockWidth + scaledMarginEnd;
        skippedHeight = 0;
        skippedCount = 0;
      }

      ctx.fill();
    }

    
    
    
    this._blocksBoundingRects.sort((a, b) => a.start > b.start ? 1 : -1);

    

    while (this._legendNode.hasChildNodes()) {
      this._legendNode.firstChild.remove();
    }
    for (let { color, label } of this.format) {
      this._createLegendItem(color, label);
    }

    return canvas;
  },

  













  buildMaskImage: function(highlights, inPixels = false, unpack = e => e.delta) {
    
    
    if (!highlights) {
      return null;
    }

    
    

    let { canvas, ctx } = this._getNamedCanvas("graph-highlights");
    let width = this._width;
    let height = this._height;

    

    let pattern = AbstractCanvasGraph.getStripePattern({
      ownerDocument: this._document,
      backgroundColor: BAR_GRAPH_HIGHLIGHTS_MASK_BACKGROUND,
      stripesColor: BAR_GRAPH_HIGHLIGHTS_MASK_STRIPES
    });
    ctx.fillStyle = pattern;
    ctx.fillRect(0, 0, width, height);

    

    let totalTicks = this._data.length;
    let firstTick = unpack(this._data[0]);
    let lastTick = unpack(this._data[totalTicks - 1]);

    for (let { start, end, top, bottom } of highlights) {
      if (!inPixels) {
        start = map(start, firstTick, lastTick, 0, width);
        end = map(end, firstTick, lastTick, 0, width);
      }
      let firstSnap = findFirst(this._blocksBoundingRects, e => e.start >= start);
      let lastSnap = findLast(this._blocksBoundingRects, e => e.start >= start && e.end <= end);

      let x1 = firstSnap ? firstSnap.start : start;
      let x2 = lastSnap ? lastSnap.end : firstSnap ? firstSnap.end : end;
      let y1 = top || 0;
      let y2 = bottom || height;
      ctx.clearRect(x1, y1, x2 - x1, y2 - y1);
    }

    return canvas;
  },

  



  _blocksBoundingRects: null,

  









  _calcMaxHeight: function({ data, dataScaleX, minBarsWidth }) {
    let maxHeight = 0;
    let prevRight = 0;
    let skippedCount = 0;
    let skippedHeight = 0;
    let scaledMarginEnd = BAR_GRAPH_BARS_MARGIN_END * this._pixelRatio;

    for (let { delta, values } of data) {
      let barRight = (delta - this.dataOffsetX) * dataScaleX;
      let barHeight = values.reduce((a, b) => a + b, 0);

      let barWidth = barRight - prevRight;
      if (barWidth < minBarsWidth) {
        skippedCount++;
        skippedHeight += barHeight;
        continue;
      }

      let averageHeight = (barHeight + skippedHeight) / (skippedCount + 1);
      maxHeight = Math.max(averageHeight, maxHeight);

      prevRight += barWidth + scaledMarginEnd;
      skippedHeight = 0;
      skippedCount = 0;
    }

    return maxHeight;
  },

  


  _createLegend: function() {
    let legendNode = this._legendNode = this._document.createElementNS(HTML_NS, "div");
    legendNode.className = "bar-graph-widget-legend";
    this._container.appendChild(legendNode);
  },

  


  _createLegendItem: function(color, label) {
    let itemNode = this._document.createElementNS(HTML_NS, "div");
    itemNode.className = "bar-graph-widget-legend-item";

    let colorNode = this._document.createElementNS(HTML_NS, "span");
    colorNode.setAttribute("view", "color");
    colorNode.setAttribute("data-index", this._legendNode.childNodes.length);
    colorNode.style.backgroundColor = color;
    colorNode.addEventListener("mouseover", this._onLegendMouseOver);
    colorNode.addEventListener("mouseout", this._onLegendMouseOut);
    colorNode.addEventListener("mousedown", this._onLegendMouseDown);
    colorNode.addEventListener("mouseup", this._onLegendMouseUp);

    this.outstandingEventListeners.push([colorNode, "mouseover", this._onLegendMouseOver]);
    this.outstandingEventListeners.push([colorNode, "mouseout", this._onLegendMouseOut]);
    this.outstandingEventListeners.push([colorNode, "mousedown", this._onLegendMouseDown]);
    this.outstandingEventListeners.push([colorNode, "mouseup", this._onLegendMouseUp]);

    let labelNode = this._document.createElementNS(HTML_NS, "span");
    labelNode.setAttribute("view", "label");
    labelNode.textContent = label;

    itemNode.appendChild(colorNode);
    itemNode.appendChild(labelNode);
    this._legendNode.appendChild(itemNode);
  },

  


  _onLegendMouseOver: function(e) {
    setNamedTimeout("bar-graph-debounce", BAR_GRAPH_LEGEND_MOUSEOVER_DEBOUNCE, () => {
      let type = e.target.dataset.index;
      let rects = this._blocksBoundingRects.filter(e => e.type == type);

      this._originalHighlights = this._mask;
      this._hasCustomHighlights = true;
      this.setMask(rects, true);

      this.emit("legend-hover", [type, rects]);
    });
  },

  


  _onLegendMouseOut: function() {
    clearNamedTimeout("bar-graph-debounce");

    if (this._hasCustomHighlights) {
      this.setMask(this._originalHighlights);
      this._hasCustomHighlights = false;
      this._originalHighlights = null;
    }

    this.emit("legend-unhover");
  },

  


  _onLegendMouseDown: function(e) {
    e.preventDefault();
    e.stopPropagation();

    let type = e.target.dataset.index;
    let rects = this._blocksBoundingRects.filter(e => e.type == type);
    let leftmost = rects[0];
    let rightmost = rects[rects.length - 1];
    if (!leftmost || !rightmost) {
      this.dropSelection();
    } else {
      this.setSelection({ start: leftmost.start, end: rightmost.end });
    }

    this.emit("legend-selection", [leftmost, rightmost]);
  },

  


  _onLegendMouseUp: function(e) {
    e.preventDefault();
    e.stopPropagation();
  }
});














AbstractCanvasGraph.createIframe = function(url, parent, callback) {
  let iframe = parent.ownerDocument.createElementNS(HTML_NS, "iframe");

  iframe.addEventListener("DOMContentLoaded", function onLoad() {
    iframe.removeEventListener("DOMContentLoaded", onLoad);
    callback(iframe);
  });

  iframe.setAttribute("frameborder", "0");
  iframe.src = url;

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

  











  _performTaskInWorker: function(task, args, transferrable) {
    let worker = this._graphUtilsWorker || new ChromeWorker(WORKER_URL);
    let id = this._graphUtilsTaskId++;
    worker.postMessage({ task, id, args }, transferrable);
    return this._waitForWorkerResponse(worker, id);
  },

  







  _waitForWorkerResponse: function(worker, id) {
    let deferred = promise.defer();

    worker.addEventListener("message", function listener({ data }) {
      if (data.id != id) {
        return;
      }
      worker.removeEventListener("message", listener);
      deferred.resolve(data);
    });

    return deferred.promise;
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




function distSquared(x0, y0, x1, y1) {
  let xs = x1 - x0;
  let ys = y1 - y0;
  return xs * xs + ys * ys;
}







function findFirst(array, predicate) {
  for (let i = 0, len = array.length; i < len; i++) {
    let element = array[i];
    if (predicate(element)) return element;
  }
}







function findLast(array, predicate) {
  for (let i = array.length - 1; i >= 0; i--) {
    let element = array[i];
    if (predicate(element)) return element;
  }
}
