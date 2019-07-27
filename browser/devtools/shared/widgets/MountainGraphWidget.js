"use strict";

const { Cc, Ci, Cu, Cr } = require("chrome");

const { Heritage } = require("resource:///modules/devtools/ViewHelpers.jsm");
const { AbstractCanvasGraph, CanvasGraphUtils } = require("devtools/shared/widgets/Graphs");

const HTML_NS = "http://www.w3.org/1999/xhtml";



const GRAPH_DAMPEN_VALUES_FACTOR = 0.9;

const GRAPH_BACKGROUND_COLOR = "#ddd";
const GRAPH_STROKE_WIDTH = 2; 
const GRAPH_STROKE_COLOR = "rgba(255,255,255,0.9)";
const GRAPH_HELPER_LINES_DASH = [5]; 
const GRAPH_HELPER_LINES_WIDTH = 1; 

const GRAPH_CLIPHEAD_LINE_COLOR = "#fff";
const GRAPH_SELECTION_LINE_COLOR = "#fff";
const GRAPH_SELECTION_BACKGROUND_COLOR = "rgba(44,187,15,0.25)";
const GRAPH_SELECTION_STRIPES_COLOR = "rgba(255,255,255,0.1)";
const GRAPH_REGION_BACKGROUND_COLOR = "transparent";
const GRAPH_REGION_STRIPES_COLOR = "rgba(237,38,85,0.2)";


































this.MountainGraphWidget = function(parent, ...args) {
  AbstractCanvasGraph.apply(this, [parent, "mountain-graph", ...args]);
};

MountainGraphWidget.prototype = Heritage.extend(AbstractCanvasGraph.prototype, {
  backgroundColor: GRAPH_BACKGROUND_COLOR,
  strokeColor: GRAPH_STROKE_COLOR,
  strokeWidth: GRAPH_STROKE_WIDTH,
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

  



  buildBackgroundImage: function() {
    let { canvas, ctx } = this._getNamedCanvas("mountain-graph-background");
    let width = this._width;
    let height = this._height;

    ctx.fillStyle = this.backgroundColor;
    ctx.fillRect(0, 0, width, height);

    return canvas;
  },

  



  buildGraphImage: function() {
    if (!this.format || !this.format.length) {
      throw "The graph format traits are mandatory to style the data source.";
    }
    let { canvas, ctx } = this._getNamedCanvas("mountain-graph-data");
    let width = this._width;
    let height = this._height;

    let totalSections = this.format.length;
    let totalTicks = this._data.length;
    let firstTick = this._data[0].delta;
    let lastTick = this._data[totalTicks - 1].delta;

    let duration = this.dataDuration || lastTick;
    let dataScaleX = this.dataScaleX = width / (duration - this.dataOffsetX);
    let dataScaleY = this.dataScaleY = height * this.dampenValuesFactor;

    

    let prevHeights = Array.from({ length: totalTicks }).fill(0);

    ctx.globalCompositeOperation = "destination-over";
    ctx.strokeStyle = this.strokeColor;
    ctx.lineWidth = this.strokeWidth * this._pixelRatio;

    for (let section = 0; section < totalSections; section++) {
      ctx.fillStyle = this.format[section].color || "#000";
      ctx.beginPath();

      for (let tick = 0; tick < totalTicks; tick++) {
        let { delta, values } = this._data[tick];
        let currX = (delta - this.dataOffsetX) * dataScaleX;
        let currY = values[section] * dataScaleY;
        let prevY = prevHeights[tick];

        if (delta == firstTick) {
          ctx.moveTo(-GRAPH_STROKE_WIDTH, height);
          ctx.lineTo(-GRAPH_STROKE_WIDTH, height - currY - prevY);
        }

        ctx.lineTo(currX, height - currY - prevY);

        if (delta == lastTick) {
          ctx.lineTo(width + GRAPH_STROKE_WIDTH, height - currY - prevY);
          ctx.lineTo(width + GRAPH_STROKE_WIDTH, height);
        }

        prevHeights[tick] += currY;
      }

      ctx.fill();
      ctx.stroke();
    }

    ctx.globalCompositeOperation = "source-over";
    ctx.lineWidth = GRAPH_HELPER_LINES_WIDTH;
    ctx.setLineDash(GRAPH_HELPER_LINES_DASH);

    

    ctx.beginPath();
    let maximumY = height * this.dampenValuesFactor;
    ctx.moveTo(0, maximumY);
    ctx.lineTo(width, maximumY);
    ctx.stroke();

    

    ctx.beginPath();
    let averageY = height / 2 * this.dampenValuesFactor;
    ctx.moveTo(0, averageY);
    ctx.lineTo(width, averageY);
    ctx.stroke();

    return canvas;
  }
});

module.exports = MountainGraphWidget;
