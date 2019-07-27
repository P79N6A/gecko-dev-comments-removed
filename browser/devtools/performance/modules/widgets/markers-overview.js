


"use strict";







const { Cc, Ci, Cu, Cr } = require("chrome");
const { AbstractCanvasGraph } = require("resource:///modules/devtools/Graphs.jsm");
const { Heritage } = require("resource:///modules/devtools/ViewHelpers.jsm");

loader.lazyRequireGetter(this, "colorUtils",
  "devtools/css-color", true);
loader.lazyRequireGetter(this, "getColor",
  "devtools/shared/theme", true);
loader.lazyRequireGetter(this, "L10N",
  "devtools/performance/global", true);
loader.lazyRequireGetter(this, "TickUtils",
  "devtools/performance/waterfall-ticks", true);

const OVERVIEW_HEADER_HEIGHT = 14; 
const OVERVIEW_ROW_HEIGHT = 11; 

const OVERVIEW_SELECTION_LINE_COLOR = "#666";
const OVERVIEW_CLIPHEAD_LINE_COLOR = "#555";

const FIND_OPTIMAL_TICK_INTERVAL_MAX_ITERS = 100;
const OVERVIEW_HEADER_TICKS_MULTIPLE = 100; 
const OVERVIEW_HEADER_TICKS_SPACING_MIN = 75; 
const OVERVIEW_HEADER_TEXT_FONT_SIZE = 9; 
const OVERVIEW_HEADER_TEXT_FONT_FAMILY = "sans-serif";
const OVERVIEW_HEADER_TEXT_PADDING_LEFT = 6; 
const OVERVIEW_HEADER_TEXT_PADDING_TOP = 1; 
const OVERVIEW_MARKERS_COLOR_STOPS = [0, 0.1, 0.75, 1];
const OVERVIEW_MARKER_WIDTH_MIN = 4; 
const OVERVIEW_GROUP_VERTICAL_PADDING = 5; 









function MarkersOverview(parent, blueprint, ...args) {
  AbstractCanvasGraph.apply(this, [parent, "markers-overview", ...args]);
  this.setTheme();
  this.setBlueprint(blueprint);
}

MarkersOverview.prototype = Heritage.extend(AbstractCanvasGraph.prototype, {
  clipheadLineColor: OVERVIEW_CLIPHEAD_LINE_COLOR,
  selectionLineColor: OVERVIEW_SELECTION_LINE_COLOR,
  headerHeight: OVERVIEW_HEADER_HEIGHT,
  rowHeight: OVERVIEW_ROW_HEIGHT,
  groupPadding: OVERVIEW_GROUP_VERTICAL_PADDING,

  


  get fixedHeight() {
    return this.headerHeight + this.rowHeight * (this._lastGroup + 1);
  },

  



  setBlueprint: function(blueprint) {
    this._paintBatches = new Map();
    this._lastGroup = 0;

    for (let type in blueprint) {
      this._paintBatches.set(type, { style: blueprint[type], batch: [] });
      this._lastGroup = Math.max(this._lastGroup, blueprint[type].group || 0);
    }
  },

  


  clearView: function() {
    this.selectionEnabled = false;
    this.dropSelection();
    this.setData({ duration: 0, markers: [] });
  },

  



  buildGraphImage: function() {
    let { markers, duration } = this._data;

    let { canvas, ctx } = this._getNamedCanvas("markers-overview-data");
    let canvasWidth = this._width;
    let canvasHeight = this._height;

    
    

    for (let marker of markers) {
      let markerType = this._paintBatches.get(marker.name);
      if (markerType) {
        markerType.batch.push(marker);
      }
    }

    

    let totalGroups = this._lastGroup + 1;
    let groupHeight = this.rowHeight * this._pixelRatio;
    let groupPadding = this.groupPadding * this._pixelRatio;
    let headerHeight = this.headerHeight * this._pixelRatio;
    let dataScale = this.dataScaleX = canvasWidth / duration;

    

    ctx.fillStyle = this.headerBackgroundColor;
    ctx.fillRect(0, 0, canvasWidth, headerHeight);

    ctx.fillStyle = this.backgroundColor;
    ctx.fillRect(0, headerHeight, canvasWidth, canvasHeight);

    

    ctx.fillStyle = this.alternatingBackgroundColor;
    ctx.beginPath();

    for (let i = 0; i < totalGroups; i += 2) {
      let top = headerHeight + i * groupHeight;
      ctx.rect(0, top, canvasWidth, groupHeight);
    }

    ctx.fill();

    

    let fontSize = OVERVIEW_HEADER_TEXT_FONT_SIZE * this._pixelRatio;
    let fontFamily = OVERVIEW_HEADER_TEXT_FONT_FAMILY;
    let textPaddingLeft = OVERVIEW_HEADER_TEXT_PADDING_LEFT * this._pixelRatio;
    let textPaddingTop = OVERVIEW_HEADER_TEXT_PADDING_TOP * this._pixelRatio;

    let tickInterval = TickUtils.findOptimalTickInterval({
      ticksMultiple: OVERVIEW_HEADER_TICKS_MULTIPLE,
      ticksSpacingMin: OVERVIEW_HEADER_TICKS_SPACING_MIN,
      dataScale: dataScale
    });

    ctx.textBaseline = "middle";
    ctx.font = fontSize + "px " + fontFamily;
    ctx.fillStyle = this.headerTextColor;
    ctx.strokeStyle = this.headerTimelineStrokeColor;
    ctx.beginPath();

    for (let x = 0; x < canvasWidth; x += tickInterval) {
      let lineLeft = x;
      let textLeft = lineLeft + textPaddingLeft;
      let time = Math.round(x / dataScale);
      let label = L10N.getFormatStr("timeline.tick", time);
      ctx.fillText(label, textLeft, headerHeight / 2 + textPaddingTop);
      ctx.moveTo(lineLeft, 0);
      ctx.lineTo(lineLeft, canvasHeight);
    }

    ctx.stroke();

    

    for (let [, { style, batch }] of this._paintBatches) {
      let top = headerHeight + style.group * groupHeight + groupPadding / 2;
      let height = groupHeight - groupPadding;

      let color = getColor(style.colorName, this.theme);
      ctx.fillStyle = color;
      ctx.beginPath();

      for (let { start, end } of batch) {
        let left = start * dataScale;
        let width = Math.max((end - start) * dataScale, OVERVIEW_MARKER_WIDTH_MIN);
        ctx.rect(left, top, width, height);
      }

      ctx.fill();

      
      
      
      batch.length = 0;
    }

    return canvas;
  },

  




  setTheme: function (theme) {
    this.theme = theme = theme || "light";
    this.backgroundColor = getColor("body-background", theme);
    this.selectionBackgroundColor = colorUtils.setAlpha(getColor("selection-background", theme), 0.25);
    this.selectionStripesColor = colorUtils.setAlpha("#fff", 0.1);
    this.headerBackgroundColor = getColor("body-background", theme);
    this.headerTextColor = getColor("body-color", theme);
    this.headerTimelineStrokeColor = colorUtils.setAlpha(getColor("body-color-alt", theme), 0.25);
    this.alternatingBackgroundColor = colorUtils.setAlpha(getColor("body-color", theme), 0.05);
  }
});

exports.MarkersOverview = MarkersOverview;
