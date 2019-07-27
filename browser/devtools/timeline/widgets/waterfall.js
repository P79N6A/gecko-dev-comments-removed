


"use strict";






const {Cc, Ci, Cu, Cr} = require("chrome");

loader.lazyRequireGetter(this, "L10N",
  "devtools/timeline/global", true);
loader.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/timeline/global", true);

loader.lazyImporter(this, "setNamedTimeout",
  "resource:///modules/devtools/ViewHelpers.jsm");
loader.lazyImporter(this, "clearNamedTimeout",
  "resource:///modules/devtools/ViewHelpers.jsm");

const HTML_NS = "http://www.w3.org/1999/xhtml";

const TIMELINE_IMMEDIATE_DRAW_MARKERS_COUNT = 30;
const TIMELINE_FLUSH_OUTSTANDING_MARKERS_DELAY = 75; 

const TIMELINE_HEADER_TICKS_MULTIPLE = 5; 
const TIMELINE_HEADER_TICKS_SPACING_MIN = 50; 
const TIMELINE_HEADER_TEXT_PADDING = 3; 

const TIMELINE_MARKER_SIDEBAR_WIDTH = 150; 
const TIMELINE_MARKER_BAR_WIDTH_MIN = 5; 

const WATERFALL_BACKGROUND_TICKS_MULTIPLE = 5; 
const WATERFALL_BACKGROUND_TICKS_SCALES = 3;
const WATERFALL_BACKGROUND_TICKS_SPACING_MIN = 10; 
const WATERFALL_BACKGROUND_TICKS_COLOR_RGB = [128, 136, 144];
const WATERFALL_BACKGROUND_TICKS_OPACITY_MIN = 32; 
const WATERFALL_BACKGROUND_TICKS_OPACITY_ADD = 32; 







function Waterfall(parent) {
  this._parent = parent;
  this._document = parent.ownerDocument;
  this._fragment = this._document.createDocumentFragment();
  this._outstandingMarkers = [];

  this._headerContents = this._document.createElement("hbox");
  this._headerContents.className = "timeline-header-contents";
  this._parent.appendChild(this._headerContents);

  this._listContents = this._document.createElement("vbox");
  this._listContents.className = "timeline-list-contents";
  this._listContents.setAttribute("flex", "1");
  this._parent.appendChild(this._listContents);

  this._isRTL = this._getRTL();

  
  this._l10n = L10N;
  this._blueprint = TIMELINE_BLUEPRINT;
  this._setNamedTimeout = setNamedTimeout;
  this._clearNamedTimeout = clearNamedTimeout;
}

Waterfall.prototype = {
  









  setData: function(markers, timeStart, timeEnd) {
    this.clearView();

    let dataScale = this._waterfallWidth / (timeEnd - timeStart);
    this._drawWaterfallBackground(dataScale);
    this._buildHeader(this._headerContents, timeStart, dataScale);
    this._buildMarkers(this._listContents, markers, timeStart, timeEnd, dataScale);
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
    this._waterfallWidth = bounds.width - TIMELINE_MARKER_SIDEBAR_WIDTH;
  },

  









  _buildHeader: function(parent, timeStart, dataScale) {
    let container = this._document.createElement("hbox");
    container.className = "timeline-header-container";
    container.setAttribute("flex", "1");

    let sidebar = this._document.createElement("hbox");
    sidebar.className = "timeline-header-sidebar theme-sidebar";
    sidebar.setAttribute("width", TIMELINE_MARKER_SIDEBAR_WIDTH);
    sidebar.setAttribute("align", "center");
    container.appendChild(sidebar);

    let name = this._document.createElement("label");
    name.className = "plain timeline-header-name";
    name.setAttribute("value", this._l10n.getStr("timeline.records"));
    sidebar.appendChild(name);

    let ticks = this._document.createElement("hbox");
    ticks.className = "timeline-header-ticks";
    ticks.setAttribute("align", "center");
    ticks.setAttribute("flex", "1");
    container.appendChild(ticks);

    let offset = this._isRTL ? this._waterfallWidth : 0;
    let direction = this._isRTL ? -1 : 1;
    let tickInterval = this._findOptimalTickInterval({
      ticksMultiple: TIMELINE_HEADER_TICKS_MULTIPLE,
      ticksSpacingMin: TIMELINE_HEADER_TICKS_SPACING_MIN,
      dataScale: dataScale
    });

    for (let x = 0; x < this._waterfallWidth; x += tickInterval) {
      let start = x + direction * TIMELINE_HEADER_TEXT_PADDING;
      let time = Math.round(timeStart + x / dataScale);
      let label = this._l10n.getFormatStr("timeline.tick", time);

      let node = this._document.createElement("label");
      node.className = "plain timeline-header-tick";
      node.style.transform = "translateX(" + (start - offset) + "px)";
      node.setAttribute("value", label);
      ticks.appendChild(node);
    }

    parent.appendChild(container);
  },

  









  _buildMarkers: function(parent, markers, timeStart, timeEnd, dataScale) {
    let processed = 0;

    for (let marker of markers) {
      if (!isMarkerInRange(marker, timeStart, timeEnd)) {
        continue;
      }
      
      
      
      let arguments_ = [this._fragment, marker, timeStart, dataScale];
      if (processed++ < TIMELINE_IMMEDIATE_DRAW_MARKERS_COUNT) {
        this._buildMarker.apply(this, arguments_);
      } else {
        this._outstandingMarkers.push(arguments_);
      }
    }

    
    
    if (!this._outstandingMarkers.length) {
      this._buildMarker(this._fragment, null);
    }
    
    else {
      this._setNamedTimeout("flush-outstanding-markers",
        TIMELINE_FLUSH_OUTSTANDING_MARKERS_DELAY,
        () => this._buildOutstandingMarkers(parent));
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
  },

  











  _buildMarker: function(parent, marker, timeStart, dataScale) {
    let container = this._document.createElement("hbox");
    container.className = "timeline-marker-container";

    if (marker) {
      this._buildMarkerSidebar(container, marker);
      this._buildMarkerWaterfall(container, marker, timeStart, dataScale);
    } else {
      this._buildMarkerSpacer(container);
      container.setAttribute("flex", "1");
      container.setAttribute("is-spacer", "");
    }

    parent.appendChild(container);
  },

  







  _buildMarkerSidebar: function(container, marker) {
    let blueprint = this._blueprint[marker.name];

    let sidebar = this._document.createElement("hbox");
    sidebar.className = "timeline-marker-sidebar theme-sidebar";
    sidebar.setAttribute("width", TIMELINE_MARKER_SIDEBAR_WIDTH);
    sidebar.setAttribute("align", "center");

    let bullet = this._document.createElement("hbox");
    bullet.className = "timeline-marker-bullet";
    bullet.style.backgroundColor = blueprint.fill;
    bullet.style.borderColor = blueprint.stroke;
    bullet.setAttribute("type", marker.name);
    sidebar.appendChild(bullet);

    let name = this._document.createElement("label");
    name.className = "plain timeline-marker-name";
    name.setAttribute("value", blueprint.label);
    sidebar.appendChild(name);

    container.appendChild(sidebar);
  },

  











  _buildMarkerWaterfall: function(container, marker, timeStart, dataScale) {
    let blueprint = this._blueprint[marker.name];

    let waterfall = this._document.createElement("hbox");
    waterfall.className = "timeline-marker-waterfall";
    waterfall.setAttribute("flex", "1");

    let start = (marker.start - timeStart) * dataScale;
    let width = (marker.end - marker.start) * dataScale;
    let offset = this._isRTL ? this._waterfallWidth : 0;

    let bar = this._document.createElement("hbox");
    bar.className = "timeline-marker-bar";
    bar.style.backgroundColor = blueprint.fill;
    bar.style.borderColor = blueprint.stroke;
    bar.style.transform = "translateX(" + (start - offset) + "px)";
    bar.setAttribute("type", marker.name);
    bar.setAttribute("width", Math.max(width, TIMELINE_MARKER_BAR_WIDTH_MIN));
    waterfall.appendChild(bar);

    container.appendChild(waterfall);
  },

  





  _buildMarkerSpacer: function(container) {
    let sidebarSpacer = this._document.createElement("spacer");
    sidebarSpacer.className = "timeline-marker-sidebar theme-sidebar";
    sidebarSpacer.setAttribute("width", TIMELINE_MARKER_SIDEBAR_WIDTH);

    let waterfallSpacer = this._document.createElement("spacer");
    waterfallSpacer.className = "timeline-marker-waterfall";
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

    while (true) {
      let scaledStep = dataScale * timingStep;
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
