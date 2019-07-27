"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");

const { Heritage, setNamedTimeout, clearNamedTimeout } = require("resource:///modules/devtools/ViewHelpers.jsm");
const { AbstractCanvasGraph, CanvasGraphUtils } = require("devtools/shared/widgets/Graphs");

const HTML_NS = "http://www.w3.org/1999/xhtml";



const GRAPH_DAMPEN_VALUES_FACTOR = 0.75;
const GRAPH_BARS_MARGIN_TOP = 1; 
const GRAPH_BARS_MARGIN_END = 1; 
const GRAPH_MIN_BARS_WIDTH = 5; 
const GRAPH_MIN_BLOCKS_HEIGHT = 1; 

const GRAPH_BACKGROUND_GRADIENT_START = "rgba(0,136,204,0.0)";
const GRAPH_BACKGROUND_GRADIENT_END = "rgba(255,255,255,0.25)";

const GRAPH_CLIPHEAD_LINE_COLOR = "#666";
const GRAPH_SELECTION_LINE_COLOR = "#555";
const GRAPH_SELECTION_BACKGROUND_COLOR = "rgba(0,136,204,0.25)";
const GRAPH_SELECTION_STRIPES_COLOR = "rgba(255,255,255,0.1)";
const GRAPH_REGION_BACKGROUND_COLOR = "transparent";
const GRAPH_REGION_STRIPES_COLOR = "rgba(237,38,85,0.2)";

const GRAPH_HIGHLIGHTS_MASK_BACKGROUND = "rgba(255,255,255,0.75)";
const GRAPH_HIGHLIGHTS_MASK_STRIPES = "rgba(255,255,255,0.5)";

const GRAPH_LEGEND_MOUSEOVER_DEBOUNCE = 50; 



































this.BarGraphWidget = function(parent, ...args) {
  AbstractCanvasGraph.apply(this, [parent, "bar-graph", ...args]);

  this.once("ready", () => {
    this._onLegendMouseOver = this._onLegendMouseOver.bind(this);
    this._onLegendMouseOut = this._onLegendMouseOut.bind(this);
    this._onLegendMouseDown = this._onLegendMouseDown.bind(this);
    this._onLegendMouseUp = this._onLegendMouseUp.bind(this);
    this._createLegend();
  });
};

BarGraphWidget.prototype = Heritage.extend(AbstractCanvasGraph.prototype, {
  clipheadLineColor: GRAPH_CLIPHEAD_LINE_COLOR,
  selectionLineColor: GRAPH_SELECTION_LINE_COLOR,
  selectionBackgroundColor: GRAPH_SELECTION_BACKGROUND_COLOR,
  selectionStripesColor: GRAPH_SELECTION_STRIPES_COLOR,
  regionBackgroundColor: GRAPH_REGION_BACKGROUND_COLOR,
  regionStripesColor: GRAPH_REGION_STRIPES_COLOR,

  




  format: null,

  


  dataOffsetX: 0,

  



  dataDuration: 0,

  



  dampenValuesFactor: GRAPH_DAMPEN_VALUES_FACTOR,

  



  minBarsWidth: GRAPH_MIN_BARS_WIDTH,

  



  minBlocksHeight: GRAPH_MIN_BLOCKS_HEIGHT,

  



  buildBackgroundImage: function() {
    let { canvas, ctx } = this._getNamedCanvas("bar-graph-background");
    let width = this._width;
    let height = this._height;

    let gradient = ctx.createLinearGradient(0, 0, 0, height);
    gradient.addColorStop(0, GRAPH_BACKGROUND_GRADIENT_START);
    gradient.addColorStop(1, GRAPH_BACKGROUND_GRADIENT_END);
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

    let duration = this.dataDuration || lastTick;
    let dataScaleX = this.dataScaleX = width / (duration - this.dataOffsetX);
    let dataScaleY = this.dataScaleY = height / this._calcMaxHeight({
      data: this._data,
      dataScaleX: dataScaleX,
      minBarsWidth: minBarsWidth
    }) * this.dampenValuesFactor;

    

    
    
    

    this._blocksBoundingRects = [];
    let prevHeight = [];
    let scaledMarginEnd = GRAPH_BARS_MARGIN_END * this._pixelRatio;
    let scaledMarginTop = GRAPH_BARS_MARGIN_TOP * this._pixelRatio;

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
            prevHeight[tick] = averageHeight + scaledMarginTop;
          } else {
            prevHeight[tick] += averageHeight + scaledMarginTop;
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
      backgroundColor: GRAPH_HIGHLIGHTS_MASK_BACKGROUND,
      stripesColor: GRAPH_HIGHLIGHTS_MASK_STRIPES
    });
    ctx.fillStyle = pattern;
    ctx.fillRect(0, 0, width, height);

    

    let totalTicks = this._data.length;
    let firstTick = unpack(this._data[0]);
    let lastTick = unpack(this._data[totalTicks - 1]);

    for (let { start, end, top, bottom } of highlights) {
      if (!inPixels) {
        start = CanvasGraphUtils.map(start, firstTick, lastTick, 0, width);
        end = CanvasGraphUtils.map(end, firstTick, lastTick, 0, width);
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
    let scaledMarginEnd = GRAPH_BARS_MARGIN_END * this._pixelRatio;

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

    let labelNode = this._document.createElementNS(HTML_NS, "span");
    labelNode.setAttribute("view", "label");
    labelNode.textContent = label;

    itemNode.appendChild(colorNode);
    itemNode.appendChild(labelNode);
    this._legendNode.appendChild(itemNode);
  },

  


  _onLegendMouseOver: function(e) {
    setNamedTimeout("bar-graph-debounce", GRAPH_LEGEND_MOUSEOVER_DEBOUNCE, () => {
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

module.exports = BarGraphWidget;
