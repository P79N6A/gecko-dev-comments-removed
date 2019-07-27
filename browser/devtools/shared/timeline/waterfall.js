


"use strict";






const {Ci, Cu} = require("chrome");

loader.lazyRequireGetter(this, "L10N",
  "devtools/shared/timeline/global", true);

loader.lazyImporter(this, "setNamedTimeout",
  "resource:///modules/devtools/ViewHelpers.jsm");
loader.lazyImporter(this, "clearNamedTimeout",
  "resource:///modules/devtools/ViewHelpers.jsm");
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");

const HTML_NS = "http://www.w3.org/1999/xhtml";

const WATERFALL_SIDEBAR_WIDTH = 150; 

const WATERFALL_IMMEDIATE_DRAW_MARKERS_COUNT = 30;
const WATERFALL_FLUSH_OUTSTANDING_MARKERS_DELAY = 75; 

const FIND_OPTIMAL_TICK_INTERVAL_MAX_ITERS = 100;
const WATERFALL_HEADER_TICKS_MULTIPLE = 5; 
const WATERFALL_HEADER_TICKS_SPACING_MIN = 50; 
const WATERFALL_HEADER_TEXT_PADDING = 3; 

const WATERFALL_BACKGROUND_TICKS_MULTIPLE = 5; 
const WATERFALL_BACKGROUND_TICKS_SCALES = 3;
const WATERFALL_BACKGROUND_TICKS_SPACING_MIN = 10; 
const WATERFALL_BACKGROUND_TICKS_COLOR_RGB = [128, 136, 144];
const WATERFALL_BACKGROUND_TICKS_OPACITY_MIN = 32; 
const WATERFALL_BACKGROUND_TICKS_OPACITY_ADD = 32; 
const WATERFALL_MARKER_BAR_WIDTH_MIN = 5; 

const WATERFALL_ROWCOUNT_ONPAGEUPDOWN = 10;











function Waterfall(parent, container, blueprint) {
  EventEmitter.decorate(this);

  this._parent = parent;
  this._document = parent.ownerDocument;
  this._container = container;
  this._fragment = this._document.createDocumentFragment();
  this._outstandingMarkers = [];

  this._headerContents = this._document.createElement("hbox");
  this._headerContents.className = "waterfall-header-contents";
  this._parent.appendChild(this._headerContents);

  this._listContents = this._document.createElement("vbox");
  this._listContents.className = "waterfall-list-contents";
  this._listContents.setAttribute("flex", "1");
  this._parent.appendChild(this._listContents);

  this.setupKeys();

  this._isRTL = this._getRTL();

  
  this._l10n = L10N;
  this._blueprint = blueprint;
  this._setNamedTimeout = setNamedTimeout;
  this._clearNamedTimeout = clearNamedTimeout;

  
  
  this._selectedRowIdx = 0;

  
  this.rowCount = WATERFALL_ROWCOUNT_ONPAGEUPDOWN;
}

Waterfall.prototype = {
  


  destroy: function() {
    this._parent = this._document = this._container = null;
  },

  







  setData: function({ markers, interval }) {
    this.clearView();
    this._markers = markers;
    this._interval = interval;

    let { startTime, endTime } = interval;
    let dataScale = this._waterfallWidth / (endTime - startTime);
    this._drawWaterfallBackground(dataScale);

    this._buildHeader(this._headerContents, startTime, dataScale);
    this._buildMarkers(this._listContents, markers, startTime, endTime, dataScale);
    this.selectRow(this._selectedRowIdx);
  },

  



  setBlueprint: function(blueprint) {
    this._blueprint = blueprint;
  },

  


  setupKeys: function() {
    let pane = this._container;
    pane.addEventListener("keydown", e => {
      if (e.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_UP) {
        e.preventDefault();
        this.selectNearestRow(this._selectedRowIdx - 1);
      }
      if (e.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_DOWN) {
        e.preventDefault();
        this.selectNearestRow(this._selectedRowIdx + 1);
      }
      if (e.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_HOME) {
        e.preventDefault();
        this.selectNearestRow(0);
      }
      if (e.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_END) {
        e.preventDefault();
        this.selectNearestRow(this._listContents.children.length);
      }
      if (e.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_PAGE_UP) {
        e.preventDefault();
        this.selectNearestRow(this._selectedRowIdx - this.rowCount);
      }
      if (e.keyCode === Ci.nsIDOMKeyEvent.DOM_VK_PAGE_DOWN) {
        e.preventDefault();
        this.selectNearestRow(this._selectedRowIdx + this.rowCount);
      }
    }, true);
  },

  


  clearView: function() {
    while (this._headerContents.hasChildNodes()) {
      this._headerContents.firstChild.remove();
    }
    while (this._listContents.hasChildNodes()) {
      this._listContents.firstChild.remove();
    }
    this._listContents.scrollTop = 0;
    this._outstandingMarkers.length = 0;
    this._clearNamedTimeout("flush-outstanding-markers");
  },

  



  recalculateBounds: function() {
    let bounds = this._parent.getBoundingClientRect();
    this._waterfallWidth = bounds.width - WATERFALL_SIDEBAR_WIDTH;
  },

  









  _buildHeader: function(parent, startTime, dataScale) {
    let container = this._document.createElement("hbox");
    container.className = "waterfall-header-container";
    container.setAttribute("flex", "1");

    let sidebar = this._document.createElement("hbox");
    sidebar.className = "waterfall-sidebar theme-sidebar";
    sidebar.setAttribute("width", WATERFALL_SIDEBAR_WIDTH);
    sidebar.setAttribute("align", "center");
    container.appendChild(sidebar);

    let name = this._document.createElement("label");
    name.className = "plain waterfall-header-name";
    name.setAttribute("value", this._l10n.getStr("timeline.records"));
    sidebar.appendChild(name);

    let ticks = this._document.createElement("hbox");
    ticks.className = "waterfall-header-ticks waterfall-background-ticks";
    ticks.setAttribute("align", "center");
    ticks.setAttribute("flex", "1");
    container.appendChild(ticks);

    let offset = this._isRTL ? this._waterfallWidth : 0;
    let direction = this._isRTL ? -1 : 1;
    let tickInterval = this._findOptimalTickInterval({
      ticksMultiple: WATERFALL_HEADER_TICKS_MULTIPLE,
      ticksSpacingMin: WATERFALL_HEADER_TICKS_SPACING_MIN,
      dataScale: dataScale
    });

    for (let x = 0; x < this._waterfallWidth; x += tickInterval) {
      let left = x + direction * WATERFALL_HEADER_TEXT_PADDING;
      let time = Math.round(x / dataScale + startTime);
      let label = this._l10n.getFormatStr("timeline.tick", time);

      let node = this._document.createElement("label");
      node.className = "plain waterfall-header-tick";
      node.style.transform = "translateX(" + (left - offset) + "px)";
      node.setAttribute("value", label);
      ticks.appendChild(node);
    }

    parent.appendChild(container);
  },

  









  _buildMarkers: function(parent, markers, startTime, endTime, dataScale) {
    let rowsCount = 0;
    let markerIdx = -1;

    for (let marker of markers) {
      markerIdx++;

      if (!isMarkerInRange(marker, startTime, endTime)) {
        continue;
      }
      if (!(marker.name in this._blueprint)) {
        continue;
      }

      
      
      
      let arguments_ = [this._fragment, marker, startTime, dataScale, markerIdx, rowsCount];
      if (rowsCount++ < WATERFALL_IMMEDIATE_DRAW_MARKERS_COUNT) {
        this._buildMarker.apply(this, arguments_);
      } else {
        this._outstandingMarkers.push(arguments_);
      }
    }

    
    
    if (!this._outstandingMarkers.length) {
      this._buildMarker(this._fragment, null);
    }
    
    else {
      let delay = WATERFALL_FLUSH_OUTSTANDING_MARKERS_DELAY;
      let func = () => this._buildOutstandingMarkers(parent);
      this._setNamedTimeout("flush-outstanding-markers", delay, func);
    }

    parent.appendChild(this._fragment);
  },

  



  _buildOutstandingMarkers: function(parent) {
    if (!this._outstandingMarkers.length) {
      return;
    }
    for (let args of this._outstandingMarkers) {
      this._buildMarker.apply(this, args);
    }
    this._outstandingMarkers.length = 0;
    parent.appendChild(this._fragment);
    this.selectRow(this._selectedRowIdx);
  },

  















  _buildMarker: function(parent, marker, startTime, dataScale, markerIdx, rowIdx) {
    let container = this._document.createElement("hbox");
    container.setAttribute("markerIdx", markerIdx);
    container.className = "waterfall-marker-container";

    if (marker) {
      this._buildMarkerSidebar(container, marker);
      this._buildMarkerWaterfall(container, marker, startTime, dataScale, markerIdx);
      container.onclick = () => this.selectRow(rowIdx);
    } else {
      this._buildMarkerSpacer(container);
      container.setAttribute("flex", "1");
      container.setAttribute("is-spacer", "");
    }

    parent.appendChild(container);
  },

  


  resetSelection: function() {
    this.selectRow(0);
  },

  





  selectRow: function(idx) {
    let prev = this._listContents.children[this._selectedRowIdx];
    if (prev) {
      prev.classList.remove("selected");
    }

    this._selectedRowIdx = idx;

    let row = this._listContents.children[idx];
    if (row && !row.hasAttribute("is-spacer")) {
      row.focus();
      row.classList.add("selected");

      let markerIdx = row.getAttribute("markerIdx");
      this.emit("selected", this._markers[markerIdx]);
      this.ensureRowIsVisible(row);
    } else {
      this.emit("unselected");
    }
  },

  





  selectNearestRow: function(idx) {
    if (this._listContents.children.length == 0) {
      return;
    }
    idx = Math.max(idx, 0);
    idx = Math.min(idx, this._listContents.children.length - 1);
    let row = this._listContents.children[idx];
    if (row && row.hasAttribute("is-spacer")) {
      if (idx > 0) {
        return this.selectNearestRow(idx - 1);
      } else {
        return;
      }
    }
    this.selectRow(idx);
  },

  





  ensureRowIsVisible: function(row) {
    let parent = row.parentNode;
    let parentRect = parent.getBoundingClientRect();
    let rowRect = row.getBoundingClientRect();
    let yDelta = rowRect.top - parentRect.top;
    if (yDelta < 0) {
      parent.scrollTop += yDelta;
    }
    yDelta = parentRect.bottom - rowRect.bottom;
    if (yDelta < 0) {
      parent.scrollTop -= yDelta;
    }
  },

  







  _buildMarkerSidebar: function(container, marker) {
    let blueprint = this._blueprint[marker.name];

    let sidebar = this._document.createElement("hbox");
    sidebar.className = "waterfall-sidebar theme-sidebar";
    sidebar.setAttribute("width", WATERFALL_SIDEBAR_WIDTH);
    sidebar.setAttribute("align", "center");

    let bullet = this._document.createElement("hbox");
    bullet.className = `waterfall-marker-bullet ${blueprint.colorName}`;
    bullet.setAttribute("type", marker.name);
    sidebar.appendChild(bullet);

    let name = this._document.createElement("label");
    name.setAttribute("crop", "end");
    name.setAttribute("flex", "1");
    name.className = "plain waterfall-marker-name";

    let label;
    if (marker.causeName) {
      label = this._l10n.getFormatStr("timeline.markerDetailFormat",
                                      blueprint.label,
                                      marker.causeName);
    } else {
      label = blueprint.label;
    }
    name.setAttribute("value", label);
    name.setAttribute("tooltiptext", label);
    sidebar.appendChild(name);

    container.appendChild(sidebar);
  },

  











  _buildMarkerWaterfall: function(container, marker, startTime, dataScale) {
    let blueprint = this._blueprint[marker.name];

    let waterfall = this._document.createElement("hbox");
    waterfall.className = "waterfall-marker-item waterfall-background-ticks";
    waterfall.setAttribute("align", "center");
    waterfall.setAttribute("flex", "1");

    let start = (marker.start - startTime) * dataScale;
    let width = (marker.end - marker.start) * dataScale;
    let offset = this._isRTL ? this._waterfallWidth : 0;

    let bar = this._document.createElement("hbox");
    bar.className = `waterfall-marker-bar ${blueprint.colorName}`;
    bar.style.transform = "translateX(" + (start - offset) + "px)";
    bar.setAttribute("type", marker.name);
    bar.setAttribute("width", Math.max(width, WATERFALL_MARKER_BAR_WIDTH_MIN));
    waterfall.appendChild(bar);

    container.appendChild(waterfall);
  },

  





  _buildMarkerSpacer: function(container) {
    let sidebarSpacer = this._document.createElement("spacer");
    sidebarSpacer.className = "waterfall-sidebar theme-sidebar";
    sidebarSpacer.setAttribute("width", WATERFALL_SIDEBAR_WIDTH);

    let waterfallSpacer = this._document.createElement("spacer");
    waterfallSpacer.className = "waterfall-marker-item waterfall-background-ticks";
    waterfallSpacer.setAttribute("flex", "1");

    container.appendChild(sidebarSpacer);
    container.appendChild(waterfallSpacer);
  },

  





  _drawWaterfallBackground: function(dataScale) {
    if (!this._canvas || !this._ctx) {
      this._canvas = this._document.createElementNS(HTML_NS, "canvas");
      this._ctx = this._canvas.getContext("2d");
    }
    let canvas = this._canvas;
    let ctx = this._ctx;

    
    let canvasWidth = canvas.width = this._waterfallWidth;
    let canvasHeight = canvas.height = 1; 

    
    let imageData = ctx.createImageData(canvasWidth, canvasHeight);
    let pixelArray = imageData.data;

    let buf = new ArrayBuffer(pixelArray.length);
    let view8bit = new Uint8ClampedArray(buf);
    let view32bit = new Uint32Array(buf);

    
    let [r, g, b] = WATERFALL_BACKGROUND_TICKS_COLOR_RGB;
    let alphaComponent = WATERFALL_BACKGROUND_TICKS_OPACITY_MIN;
    let tickInterval = this._findOptimalTickInterval({
      ticksMultiple: WATERFALL_BACKGROUND_TICKS_MULTIPLE,
      ticksSpacingMin: WATERFALL_BACKGROUND_TICKS_SPACING_MIN,
      dataScale: dataScale
    });

    
    for (let i = 1; i <= WATERFALL_BACKGROUND_TICKS_SCALES; i++) {
      let increment = tickInterval * Math.pow(2, i);
      for (let x = 0; x < canvasWidth; x += increment) {
        let position = x | 0;
        view32bit[position] = (alphaComponent << 24) | (b << 16) | (g << 8) | r;
      }
      alphaComponent += WATERFALL_BACKGROUND_TICKS_OPACITY_ADD;
    }

    
    pixelArray.set(view8bit);
    ctx.putImageData(imageData, 0, 0);
    this._document.mozSetImageElement("waterfall-background", canvas);
  },

  







  _findOptimalTickInterval: function({ ticksMultiple, ticksSpacingMin, dataScale }) {
    let timingStep = ticksMultiple;
    let maxIters = FIND_OPTIMAL_TICK_INTERVAL_MAX_ITERS;
    let numIters = 0;

    if (dataScale > ticksSpacingMin) {
      return dataScale;
    }

    while (true) {
      let scaledStep = dataScale * timingStep;
      if (++numIters > maxIters) {
        return scaledStep;
      }
      if (scaledStep < ticksSpacingMin) {
        timingStep <<= 1;
        continue;
      }
      return scaledStep;
    }
  },

  



  _getRTL: function() {
    let win = this._document.defaultView;
    let doc = this._document.documentElement;
    return win.getComputedStyle(doc, null).direction == "rtl";
  }
};













function isMarkerInRange(e, start, end) {
  return (e.start >= start && e.end <= end) || 
         (e.start < start && e.end > end) || 
         (e.start < start && e.end >= start && e.end <= end) || 
         (e.end > end && e.start >= start && e.start <= end); 
}

exports.Waterfall = Waterfall;
