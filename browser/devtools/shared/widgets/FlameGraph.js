


"use strict";

const { ViewHelpers } = require("resource:///modules/devtools/ViewHelpers.jsm");
const { AbstractCanvasGraph, GraphArea, GraphAreaDragger } = require("resource:///modules/devtools/Graphs.jsm");
const { Promise } = require("resource://gre/modules/Promise.jsm");
const { Task } = require("resource://gre/modules/Task.jsm");
const { getColor } = require("devtools/shared/theme");
const EventEmitter = require("devtools/toolkit/event-emitter");
const FrameUtils = require("devtools/shared/profiler/frame-utils");

const HTML_NS = "http://www.w3.org/1999/xhtml";
const GRAPH_SRC = "chrome://browser/content/devtools/graphs-frame.xhtml";
const L10N = new ViewHelpers.L10N();

const GRAPH_RESIZE_EVENTS_DRAIN = 100; 

const GRAPH_WHEEL_ZOOM_SENSITIVITY = 0.00035;
const GRAPH_WHEEL_SCROLL_SENSITIVITY = 0.5;
const GRAPH_MIN_SELECTION_WIDTH = 0.001; 

const GRAPH_HORIZONTAL_PAN_THRESHOLD = 10; 
const GRAPH_VERTICAL_PAN_THRESHOLD = 30; 

const FIND_OPTIMAL_TICK_INTERVAL_MAX_ITERS = 100;
const TIMELINE_TICKS_MULTIPLE = 5; 
const TIMELINE_TICKS_SPACING_MIN = 75; 

const OVERVIEW_HEADER_HEIGHT = 16; 
const OVERVIEW_HEADER_TEXT_FONT_SIZE = 9; 
const OVERVIEW_HEADER_TEXT_FONT_FAMILY = "sans-serif";
const OVERVIEW_HEADER_TEXT_PADDING_LEFT = 6; 
const OVERVIEW_HEADER_TEXT_PADDING_TOP = 5; 
const OVERVIEW_HEADER_TIMELINE_STROKE_COLOR = "rgba(128, 128, 128, 0.5)";

const FLAME_GRAPH_BLOCK_BORDER = 1; 
const FLAME_GRAPH_BLOCK_TEXT_FONT_SIZE = 9; 
const FLAME_GRAPH_BLOCK_TEXT_FONT_FAMILY = "sans-serif";
const FLAME_GRAPH_BLOCK_TEXT_PADDING_TOP = 0; 
const FLAME_GRAPH_BLOCK_TEXT_PADDING_LEFT = 3; 
const FLAME_GRAPH_BLOCK_TEXT_PADDING_RIGHT = 3; 
















































function FlameGraph(parent, sharpness) {
  EventEmitter.decorate(this);

  this._parent = parent;
  this._ready = Promise.defer();

  this.setTheme();

  AbstractCanvasGraph.createIframe(GRAPH_SRC, parent, iframe => {
    this._iframe = iframe;
    this._window = iframe.contentWindow;
    this._document = iframe.contentDocument;
    this._pixelRatio = sharpness || this._window.devicePixelRatio;

    let container = this._container = this._document.getElementById("graph-container");
    container.className = "flame-graph-widget-container graph-widget-container";

    let canvas = this._canvas = this._document.getElementById("graph-canvas");
    canvas.className = "flame-graph-widget-canvas graph-widget-canvas";

    let bounds = parent.getBoundingClientRect();
    bounds.width = this.fixedWidth || bounds.width;
    bounds.height = this.fixedHeight || bounds.height;
    iframe.setAttribute("width", bounds.width);
    iframe.setAttribute("height", bounds.height);

    this._width = canvas.width = bounds.width * this._pixelRatio;
    this._height = canvas.height = bounds.height * this._pixelRatio;
    this._ctx = canvas.getContext("2d");

    this._bounds = new GraphArea();
    this._selection = new GraphArea();
    this._selectionDragger = new GraphAreaDragger();
    this._verticalOffset = 0;
    this._verticalOffsetDragger = new GraphAreaDragger(0);

    
    
    
    this._textWidthsCache = {};

    let fontSize = FLAME_GRAPH_BLOCK_TEXT_FONT_SIZE * this._pixelRatio;
    let fontFamily = FLAME_GRAPH_BLOCK_TEXT_FONT_FAMILY;
    this._ctx.font = fontSize + "px " + fontFamily;
    this._averageCharWidth = this._calcAverageCharWidth();
    this._overflowCharWidth = this._getTextWidth(this.overflowChar);

    this._onAnimationFrame = this._onAnimationFrame.bind(this);
    this._onMouseMove = this._onMouseMove.bind(this);
    this._onMouseDown = this._onMouseDown.bind(this);
    this._onMouseUp = this._onMouseUp.bind(this);
    this._onMouseWheel = this._onMouseWheel.bind(this);
    this._onResize = this._onResize.bind(this);
    this.refresh = this.refresh.bind(this);

    this._window.addEventListener("mousemove", this._onMouseMove);
    this._window.addEventListener("mousedown", this._onMouseDown);
    this._window.addEventListener("mouseup", this._onMouseUp);
    this._window.addEventListener("MozMousePixelScroll", this._onMouseWheel);

    let ownerWindow = this._parent.ownerDocument.defaultView;
    ownerWindow.addEventListener("resize", this._onResize);

    this._animationId = this._window.requestAnimationFrame(this._onAnimationFrame);

    this._ready.resolve(this);
    this.emit("ready", this);
  });
}

FlameGraph.prototype = {
  



  get width() {
    return this._width;
  },
  get height() {
    return this._height;
  },

  


  ready: function() {
    return this._ready.promise;
  },

  


  destroy: Task.async(function*() {
    yield this.ready();

    this._window.removeEventListener("mousemove", this._onMouseMove);
    this._window.removeEventListener("mousedown", this._onMouseDown);
    this._window.removeEventListener("mouseup", this._onMouseUp);
    this._window.removeEventListener("MozMousePixelScroll", this._onMouseWheel);

    let ownerWindow = this._parent.ownerDocument.defaultView;
    if (ownerWindow) {
      ownerWindow.removeEventListener("resize", this._onResize);
    }

    this._window.cancelAnimationFrame(this._animationId);
    this._iframe.remove();

    this._bounds = null;
    this._selection = null;
    this._selectionDragger = null;
    this._verticalOffset = null;
    this._verticalOffsetDragger = null;
    this._textWidthsCache = null;

    this._data = null;

    this.emit("destroyed");
  }),

  



  fixedWidth: null,
  fixedHeight: null,

  


  horizontalPanThreshold: GRAPH_HORIZONTAL_PAN_THRESHOLD,
  verticalPanThreshold: GRAPH_VERTICAL_PAN_THRESHOLD,

  



  timelineTickUnits: "",

  



  overflowChar: L10N.ellipsis,

  








  setData: function({ data, bounds, visible }) {
    this._data = data;
    this.setOuterBounds(bounds);
    this.setViewRange(visible || bounds);
  },

  







  setDataWhenReady: Task.async(function*(data) {
    yield this.ready();
    this.setData(data);
  }),

  



  hasData: function() {
    return !!this._data;
  },

  



  setOuterBounds: function({ startTime, endTime }) {
    this._bounds.start = startTime * this._pixelRatio;
    this._bounds.end = endTime * this._pixelRatio;
    this._shouldRedraw = true;
  },

  



  setViewRange: function({ startTime, endTime }, verticalOffset = 0) {
    this._selection.start = startTime * this._pixelRatio;
    this._selection.end = endTime * this._pixelRatio;
    this._verticalOffset = verticalOffset * this._pixelRatio;
    this._shouldRedraw = true;
  },

  



  getOuterBounds: function() {
    return {
      startTime: this._bounds.start / this._pixelRatio,
      endTime: this._bounds.end / this._pixelRatio
    };
  },

  



  getViewRange: function() {
    return {
      startTime: this._selection.start / this._pixelRatio,
      endTime: this._selection.end / this._pixelRatio,
      verticalOffset: this._verticalOffset / this._pixelRatio
    };
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

    this._shouldRedraw = true;
    this.emit("refresh");
  },

  




  setTheme: function (theme) {
    theme = theme || "light";
    this.overviewHeaderBackgroundColor = getColor("body-background", theme);
    this.overviewHeaderTextColor = getColor("body-color", theme);
    
    this.blockTextColor = getColor(theme === "dark" ? "selection-color" : "body-color", theme);
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
    let canvasWidth = this._width;
    let canvasHeight = this._height;
    ctx.clearRect(0, 0, canvasWidth, canvasHeight);

    let selection = this._selection;
    let selectionWidth = selection.end - selection.start;
    let selectionScale = canvasWidth / selectionWidth;
    this._drawTicks(selection.start, selectionScale);
    this._drawPyramid(this._data, this._verticalOffset, selection.start, selectionScale);
    this._drawHeader(selection.start, selectionScale);

    this._shouldRedraw = false;
  },

  






  _drawHeader: function(dataOffset, dataScale) {
    let ctx = this._ctx;
    let canvasWidth = this._width;
    let headerHeight = OVERVIEW_HEADER_HEIGHT * this._pixelRatio;

    ctx.fillStyle = this.overviewHeaderBackgroundColor;
    ctx.fillRect(0, 0, canvasWidth, headerHeight);

    this._drawTicks(dataOffset, dataScale, { from: 0, to: headerHeight, renderText: true });
  },

  








  _drawTicks: function(dataOffset, dataScale, options) {
    let { from, to, renderText }  = options || {};
    let ctx = this._ctx;
    let canvasWidth = this._width;
    let canvasHeight = this._height;
    let scaledOffset = dataOffset * dataScale;

    let fontSize = OVERVIEW_HEADER_TEXT_FONT_SIZE * this._pixelRatio;
    let fontFamily = OVERVIEW_HEADER_TEXT_FONT_FAMILY;
    let textPaddingLeft = OVERVIEW_HEADER_TEXT_PADDING_LEFT * this._pixelRatio;
    let textPaddingTop = OVERVIEW_HEADER_TEXT_PADDING_TOP * this._pixelRatio;
    let tickInterval = this._findOptimalTickInterval(dataScale);

    ctx.textBaseline = "top";
    ctx.font = fontSize + "px " + fontFamily;
    ctx.fillStyle = this.overviewHeaderTextColor;
    ctx.strokeStyle = OVERVIEW_HEADER_TIMELINE_STROKE_COLOR;
    ctx.beginPath();

    for (let x = -scaledOffset % tickInterval; x < canvasWidth; x += tickInterval) {
      let lineLeft = x;
      let textLeft = lineLeft + textPaddingLeft;
      let time = Math.round((x / dataScale + dataOffset) / this._pixelRatio);
      let label = time + " " + this.timelineTickUnits;
      if (renderText) {
        ctx.fillText(label, textLeft, textPaddingTop);
      }
      ctx.moveTo(lineLeft, from || 0);
      ctx.lineTo(lineLeft, to || canvasHeight);
    }

    ctx.stroke();
  },

  










  _drawPyramid: function(dataSource, verticalOffset, dataOffset, dataScale) {
    let ctx = this._ctx;

    let fontSize = FLAME_GRAPH_BLOCK_TEXT_FONT_SIZE * this._pixelRatio;
    let fontFamily = FLAME_GRAPH_BLOCK_TEXT_FONT_FAMILY;
    let visibleBlocksInfo = this._drawPyramidFill(dataSource, verticalOffset, dataOffset, dataScale);

    ctx.textBaseline = "middle";
    ctx.font = fontSize + "px " + fontFamily;
    ctx.fillStyle = this.blockTextColor;

    this._drawPyramidText(visibleBlocksInfo, verticalOffset, dataOffset, dataScale);
  },

  



  _drawPyramidFill: function(dataSource, verticalOffset, dataOffset, dataScale) {
    let visibleBlocksInfoStore = [];
    let minVisibleBlockWidth = this._overflowCharWidth;

    for (let { color, blocks } of dataSource) {
      this._drawBlocksFill(
        color, blocks, verticalOffset, dataOffset, dataScale,
        visibleBlocksInfoStore, minVisibleBlockWidth);
    }

    return visibleBlocksInfoStore;
  },

  



  _drawPyramidText: function(blocksInfo, verticalOffset, dataOffset, dataScale) {
    for (let { block, rect } of blocksInfo) {
      this._drawBlockText(block, rect, verticalOffset, dataOffset, dataScale);
    }
  },

  




















  _drawBlocksFill: function(
    color, blocks, verticalOffset, dataOffset, dataScale,
    visibleBlocksInfoStore, minVisibleBlockWidth)
  {
    let ctx = this._ctx;
    let canvasWidth = this._width;
    let canvasHeight = this._height;
    let scaledOffset = dataOffset * dataScale;

    ctx.fillStyle = color;
    ctx.beginPath();

    for (let block of blocks) {
      let { x, y, width, height } = block;
      let rectLeft = x * this._pixelRatio * dataScale - scaledOffset;
      let rectTop = (y - verticalOffset + OVERVIEW_HEADER_HEIGHT) * this._pixelRatio;
      let rectWidth = width * this._pixelRatio * dataScale;
      let rectHeight = height * this._pixelRatio;

      if (rectLeft > canvasWidth || 
          rectLeft < -rectWidth ||  
          rectTop > canvasHeight || 
          rectTop < -rectHeight) {  
        continue;
      }

      
      
      if (rectLeft < 0) {
        rectWidth += rectLeft;
        rectLeft = 0;
      }

      
      if (rectWidth <= FLAME_GRAPH_BLOCK_BORDER ||
          rectHeight <= FLAME_GRAPH_BLOCK_BORDER) {
        continue;
      }

      ctx.rect(
        rectLeft, rectTop,
        rectWidth - FLAME_GRAPH_BLOCK_BORDER,
        rectHeight - FLAME_GRAPH_BLOCK_BORDER);

      
      
      if (rectWidth > minVisibleBlockWidth) {
        visibleBlocksInfoStore.push({
          block: block,
          rect: { rectLeft, rectTop, rectWidth, rectHeight }
        });
      }
    }

    ctx.fill();
  },

  
















  _drawBlockText: function(block, rect, verticalOffset, dataOffset, dataScale) {
    let ctx = this._ctx;
    let scaledOffset = dataOffset * dataScale;

    let { x, y, width, height, text } = block;
    let { rectLeft, rectTop, rectWidth, rectHeight } = rect;

    let paddingTop = FLAME_GRAPH_BLOCK_TEXT_PADDING_TOP * this._pixelRatio;
    let paddingLeft = FLAME_GRAPH_BLOCK_TEXT_PADDING_LEFT * this._pixelRatio;
    let paddingRight = FLAME_GRAPH_BLOCK_TEXT_PADDING_RIGHT * this._pixelRatio;
    let totalHorizontalPadding = paddingLeft + paddingRight;

    
    
    if (rectLeft < 0) {
      rectWidth += rectLeft;
      rectLeft = 0;
    }

    let textLeft = rectLeft + paddingLeft;
    let textTop = rectTop + rectHeight / 2 + paddingTop;
    let textAvailableWidth = rectWidth - totalHorizontalPadding;

    
    
    let fittedText = this._getFittedText(text, textAvailableWidth);
    if (fittedText.length < 1) {
      return;
    }

    ctx.fillText(fittedText, textLeft, textTop);
  },

  




  _textWidthsCache: null,
  _overflowCharWidth: null,
  _averageCharWidth: null,

  








  _getTextWidth: function(text) {
    let cachedWidth = this._textWidthsCache[text];
    if (cachedWidth) {
      return cachedWidth;
    }
    let metrics = this._ctx.measureText(text);
    return (this._textWidthsCache[text] = metrics.width);
  },

  








  _getTextWidthApprox: function(text) {
    return text.length * this._averageCharWidth;
  },

  







  _calcAverageCharWidth: function() {
    let letterWidthsSum = 0;
    let start = 32; 
    let end = 123; 

    for (let i = start; i < end; i++) {
      let char = String.fromCharCode(i);
      letterWidthsSum += this._getTextWidth(char);
    }

    return letterWidthsSum / (end - start);
  },

  










  _getFittedText: function(text, maxWidth) {
    let textWidth = this._getTextWidth(text);
    if (textWidth < maxWidth) {
      return text;
    }
    if (this._overflowCharWidth > maxWidth) {
      return "";
    }
    for (let i = 1, len = text.length; i <= len; i++) {
      let trimmedText = text.substring(0, len - i);
      let trimmedWidth = this._getTextWidthApprox(trimmedText) + this._overflowCharWidth;
      if (trimmedWidth < maxWidth) {
        return trimmedText + this.overflowChar;
      }
    }
    return "";
  },

  


  _onMouseMove: function(e) {
    let offset = this._getContainerOffset();
    let mouseX = (e.clientX - offset.left) * this._pixelRatio;
    let mouseY = (e.clientY - offset.top) * this._pixelRatio;

    let canvasWidth = this._width;
    let canvasHeight = this._height;

    let selection = this._selection;
    let selectionWidth = selection.end - selection.start;
    let selectionScale = canvasWidth / selectionWidth;

    let horizDrag = this._selectionDragger;
    let vertDrag = this._verticalOffsetDragger;

    
    
    
    if (!this._horizontalDragEnabled && !this._verticalDragEnabled) {
      let horizDiff = Math.abs(horizDrag.origin - mouseX);
      if (horizDiff > this.horizontalPanThreshold) {
        this._horizontalDragDirection = Math.sign(horizDrag.origin - mouseX);
        this._horizontalDragEnabled = true;
      }
      let vertDiff = Math.abs(vertDrag.origin - mouseY);
      if (vertDiff > this.verticalPanThreshold) {
        this._verticalDragDirection = Math.sign(vertDrag.origin - mouseY);
        this._verticalDragEnabled = true;
      }
    }

    if (horizDrag.origin != null && this._horizontalDragEnabled) {
      let relativeX = mouseX + this._horizontalDragDirection * this.horizontalPanThreshold;
      selection.start = horizDrag.anchor.start + (horizDrag.origin - relativeX) / selectionScale;
      selection.end = horizDrag.anchor.end + (horizDrag.origin - relativeX) / selectionScale;
      this._normalizeSelectionBounds();
      this._shouldRedraw = true;
      this.emit("selecting");
    }

    if (vertDrag.origin != null && this._verticalDragEnabled) {
      let relativeY = mouseY + this._verticalDragDirection * this.verticalPanThreshold;
      this._verticalOffset = vertDrag.anchor + (vertDrag.origin - relativeY) / this._pixelRatio;
      this._normalizeVerticalOffset();
      this._shouldRedraw = true;
      this.emit("panning-vertically");
    }
  },

  


  _onMouseDown: function(e) {
    let offset = this._getContainerOffset();
    let mouseX = (e.clientX - offset.left) * this._pixelRatio;
    let mouseY = (e.clientY - offset.top) * this._pixelRatio;

    this._selectionDragger.origin = mouseX;
    this._selectionDragger.anchor.start = this._selection.start;
    this._selectionDragger.anchor.end = this._selection.end;

    this._verticalOffsetDragger.origin = mouseY;
    this._verticalOffsetDragger.anchor = this._verticalOffset;

    this._horizontalDragEnabled = false;
    this._verticalDragEnabled = false;

    this._canvas.setAttribute("input", "adjusting-view-area");
  },

  


  _onMouseUp: function() {
    this._selectionDragger.origin = null;
    this._verticalOffsetDragger.origin = null;
    this._horizontalDragEnabled = false;
    this._horizontalDragDirection = 0;
    this._verticalDragEnabled = false;
    this._verticalDragDirection = 0;
    this._canvas.removeAttribute("input");
  },

  


  _onMouseWheel: function(e) {
    let offset = this._getContainerOffset();
    let mouseX = (e.clientX - offset.left) * this._pixelRatio;

    let canvasWidth = this._width;
    let canvasHeight = this._height;

    let selection = this._selection;
    let selectionWidth = selection.end - selection.start;
    let selectionScale = canvasWidth / selectionWidth;

    switch (e.axis) {
      case e.VERTICAL_AXIS: {
        let distFromStart = mouseX;
        let distFromEnd = canvasWidth - mouseX;
        let vector = e.detail * GRAPH_WHEEL_ZOOM_SENSITIVITY / selectionScale;
        selection.start -= distFromStart * vector;
        selection.end += distFromEnd * vector;
        break;
      }
      case e.HORIZONTAL_AXIS: {
        let vector = e.detail * GRAPH_WHEEL_SCROLL_SENSITIVITY / selectionScale;
        selection.start += vector;
        selection.end += vector;
        break;
      }
    }

    this._normalizeSelectionBounds();
    this._shouldRedraw = true;
    this.emit("selecting");
  },

  




  _normalizeSelectionBounds: function() {
    let boundsStart = this._bounds.start;
    let boundsEnd = this._bounds.end;
    let selectionStart = this._selection.start;
    let selectionEnd = this._selection.end;

    if (selectionStart < boundsStart) {
      selectionStart = boundsStart;
    }
    if (selectionEnd < boundsStart) {
      selectionStart = boundsStart;
      selectionEnd = GRAPH_MIN_SELECTION_WIDTH;
    }
    if (selectionEnd > boundsEnd) {
      selectionEnd = boundsEnd;
    }
    if (selectionStart > boundsEnd) {
      selectionEnd = boundsEnd;
      selectionStart = boundsEnd - GRAPH_MIN_SELECTION_WIDTH;
    }
    if (selectionEnd - selectionStart < GRAPH_MIN_SELECTION_WIDTH) {
      let midPoint = (selectionStart + selectionEnd) / 2;
      selectionStart = midPoint - GRAPH_MIN_SELECTION_WIDTH / 2;
      selectionEnd = midPoint + GRAPH_MIN_SELECTION_WIDTH / 2;
    }

    this._selection.start = selectionStart;
    this._selection.end = selectionEnd;
  },

  



  _normalizeVerticalOffset: function() {
    this._verticalOffset = Math.max(this._verticalOffset, 0);
  },

  






  _findOptimalTickInterval: function(dataScale) {
    let timingStep = TIMELINE_TICKS_MULTIPLE;
    let spacingMin = TIMELINE_TICKS_SPACING_MIN * this._pixelRatio;
    let maxIters = FIND_OPTIMAL_TICK_INTERVAL_MAX_ITERS;
    let numIters = 0;

    if (dataScale > spacingMin) {
      return dataScale;
    }

    while (true) {
      let scaledStep = dataScale * timingStep;
      if (++numIters > maxIters) {
        return scaledStep;
      }
      if (scaledStep < spacingMin) {
        timingStep <<= 1;
        continue;
      }
      return scaledStep;
    }
  },

  





  _getContainerOffset: function() {
    let node = this._canvas;
    let x = 0;
    let y = 0;

    while ((node = node.offsetParent)) {
      x += node.offsetLeft;
      y += node.offsetTop;
    }

    return { left: x, top: y };
  },

  


  _onResize: function() {
    if (this.hasData()) {
      setNamedTimeout(this._uid, GRAPH_RESIZE_EVENTS_DRAIN, this.refresh);
    }
  }
};

const FLAME_GRAPH_BLOCK_HEIGHT = 12; 

const PALLETTE_SIZE = 10;
const PALLETTE_HUE_OFFSET = Math.random() * 90;
const PALLETTE_HUE_RANGE = 270;
const PALLETTE_SATURATION = 100;
const PALLETTE_BRIGHTNESS = 65;
const PALLETTE_OPACITY = 0.55;

const COLOR_PALLETTE = Array.from(Array(PALLETTE_SIZE)).map((_, i) => "hsla" +
  "(" + ((PALLETTE_HUE_OFFSET + (i / PALLETTE_SIZE * PALLETTE_HUE_RANGE))|0 % 360) +
  "," + PALLETTE_SATURATION + "%" +
  "," + PALLETTE_BRIGHTNESS + "%" +
  "," + PALLETTE_OPACITY +
  ")"
);





let FlameGraphUtils = {
  _cache: new WeakMap(),

  
























  createFlameGraphDataFromSamples: function(samples, options = {}, out = []) {
    let cached = this._cache.get(samples);
    if (cached) {
      return cached;
    }

    
    

    let buckets = new Map();

    for (let color of COLOR_PALLETTE) {
      buckets.set(color, []);
    }

    

    let prevTime = 0;
    let prevFrames = [];

    for (let { frames, time } of samples) {
      let frameIndex = 0;

      
      
      if (options.flattenRecursion) {
        frames = frames.filter(this._isConsecutiveDuplicate);
      }

      
      
      
      if (options.filterFrames) {
        frames = frames.filter(options.filterFrames);
      }

      
      if (options.invertStack) {
        frames.reverse();
      }

      
      if (options.showIdleBlocks && frames.length == 0) {
        frames = [{ location: options.showIdleBlocks || "", idle: true }];
      }

      for (let frame of frames) {
        let { location } = frame;
        let prevFrame = prevFrames[frameIndex];

        
        
        if (prevFrame && prevFrame.srcData.rawLocation == location) {
          prevFrame.width = (time - prevFrame.srcData.startTime);
        }
        
        
        else {
          let hash = this._getStringHash(location);
          let color = COLOR_PALLETTE[hash % PALLETTE_SIZE];
          let bucket = buckets.get(color);

          bucket.push(prevFrames[frameIndex] = {
            srcData: { startTime: prevTime, rawLocation: location },
            x: prevTime,
            y: frameIndex * FLAME_GRAPH_BLOCK_HEIGHT,
            width: time - prevTime,
            height: FLAME_GRAPH_BLOCK_HEIGHT,
            text: this._formatLabel(frame)
          });
        }

        frameIndex++;
      }

      
      
      prevFrames.length = frameIndex;
      prevTime = time;
    }

    
    

    for (let [color, blocks] of buckets) {
      out.push({ color, blocks });
    }

    this._cache.set(samples, out);
    return out;
  },

  



  removeFromCache: function(source) {
    this._cache.delete(source);
  },

  











  _isConsecutiveDuplicate: function(e, index, array) {
    return index < array.length - 1 && e.location != array[index + 1].location;
  },

  





  _getStringHash: function(input) {
    const STRING_HASH_PRIME1 = 7;
    const STRING_HASH_PRIME2 = 31;

    let hash = STRING_HASH_PRIME1;

    for (let i = 0, len = input.length; i < len; i++) {
      hash *= STRING_HASH_PRIME2;
      hash += input.charCodeAt(i);

      if (hash > Number.MAX_SAFE_INTEGER / STRING_HASH_PRIME2) {
        return hash;
      }
    }

    return hash;
  },

  






  _formatLabel: function (frame) {
    
    
    if (frame.idle) {
      return frame.location;
    }

    let { functionName, fileName, line } = FrameUtils.parseLocation(frame);
    let label = functionName;

    if (fileName) {
      label += ` (${fileName}${line != null ? (":" + line) : ""})`;
    }

    return label;
  }
};

exports.FlameGraph = FlameGraph;
exports.FlameGraphUtils = FlameGraphUtils;
exports.FLAME_GRAPH_BLOCK_HEIGHT = FLAME_GRAPH_BLOCK_HEIGHT;
