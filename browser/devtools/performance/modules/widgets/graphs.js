


"use strict";





const { Cc, Ci, Cu, Cr } = require("chrome");
const { Task } = require("resource://gre/modules/Task.jsm");
const { Heritage } = require("resource:///modules/devtools/ViewHelpers.jsm");
const LineGraphWidget = require("devtools/shared/widgets/LineGraphWidget");
const { BarGraphWidget } = require("devtools/shared/widgets/Graphs");
const { CanvasGraphUtils } = require("devtools/shared/widgets/Graphs");

loader.lazyRequireGetter(this, "promise");
loader.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");

loader.lazyRequireGetter(this, "colorUtils",
  "devtools/css-color", true);
loader.lazyRequireGetter(this, "getColor",
  "devtools/shared/theme", true);
loader.lazyRequireGetter(this, "ProfilerGlobal",
  "devtools/performance/global");
loader.lazyRequireGetter(this, "TimelineGlobal",
  "devtools/performance/global");
loader.lazyRequireGetter(this, "MarkersOverview",
  "devtools/performance/markers-overview", true);




const HEIGHT = 35; 
const STROKE_WIDTH = 1; 
const DAMPEN_VALUES = 0.95;
const CLIPHEAD_LINE_COLOR = "#666";
const SELECTION_LINE_COLOR = "#555";
const SELECTION_BACKGROUND_COLOR_NAME = "highlight-blue";
const FRAMERATE_GRAPH_COLOR_NAME = "highlight-green";
const MEMORY_GRAPH_COLOR_NAME = "highlight-blue";




const MARKERS_GRAPH_HEADER_HEIGHT = 14; 
const MARKERS_GRAPH_ROW_HEIGHT = 10; 
const MARKERS_GROUP_VERTICAL_PADDING = 4; 









function PerformanceGraph(parent, metric) {
  LineGraphWidget.call(this, parent, { metric });
  this.setTheme();
}

PerformanceGraph.prototype = Heritage.extend(LineGraphWidget.prototype, {
  strokeWidth: STROKE_WIDTH,
  dampenValuesFactor: DAMPEN_VALUES,
  fixedHeight: HEIGHT,
  clipheadLineColor: CLIPHEAD_LINE_COLOR,
  selectionLineColor: SELECTION_LINE_COLOR,
  withTooltipArrows: false,
  withFixedTooltipPositions: true,

  


  clearView: function() {
    this.selectionEnabled = false;
    this.dropSelection();
    this.setData([]);
  },

  




  setTheme: function (theme) {
    theme = theme || "light";
    let mainColor = getColor(this.mainColor || "highlight-blue", theme);
    this.backgroundColor = getColor("body-background", theme);
    this.strokeColor = mainColor;
    this.backgroundGradientStart = colorUtils.setAlpha(mainColor, 0.2);
    this.backgroundGradientEnd = colorUtils.setAlpha(mainColor, 0.2);
    this.selectionBackgroundColor = colorUtils.setAlpha(getColor(SELECTION_BACKGROUND_COLOR_NAME, theme), 0.25);
    this.selectionStripesColor = "rgba(255, 255, 255, 0.1)";
    this.maximumLineColor = colorUtils.setAlpha(mainColor, 0.4);
    this.averageLineColor = colorUtils.setAlpha(mainColor, 0.7);
    this.minimumLineColor = colorUtils.setAlpha(mainColor, 0.9);
  }
});







function FramerateGraph(parent) {
  PerformanceGraph.call(this, parent, ProfilerGlobal.L10N.getStr("graphs.fps"));
}

FramerateGraph.prototype = Heritage.extend(PerformanceGraph.prototype, {
  mainColor: FRAMERATE_GRAPH_COLOR_NAME,
  setPerformanceData: function ({ duration, ticks }, resolution) {
    this.dataDuration = duration;
    return this.setDataFromTimestamps(ticks, resolution, duration);
  }
});







function MemoryGraph(parent) {
  PerformanceGraph.call(this, parent, TimelineGlobal.L10N.getStr("graphs.memory"));
}

MemoryGraph.prototype = Heritage.extend(PerformanceGraph.prototype, {
  mainColor: MEMORY_GRAPH_COLOR_NAME,
  setPerformanceData: function ({ duration, memory }) {
    this.dataDuration = duration;
    return this.setData(memory);
  }
});

function TimelineGraph(parent, filter) {
  MarkersOverview.call(this, parent, filter);
}

TimelineGraph.prototype = Heritage.extend(MarkersOverview.prototype, {
  headerHeight: MARKERS_GRAPH_HEADER_HEIGHT,
  rowHeight: MARKERS_GRAPH_ROW_HEIGHT,
  groupPadding: MARKERS_GROUP_VERTICAL_PADDING,
  setPerformanceData: MarkersOverview.prototype.setData
});






const GRAPH_DEFINITIONS = {
  memory: {
    constructor: MemoryGraph,
    selector: "#memory-overview",
  },
  framerate: {
    constructor: FramerateGraph,
    selector: "#time-framerate",
  },
  timeline: {
    constructor: TimelineGraph,
    selector: "#markers-overview",
    primaryLink: true
  }
};










function GraphsController ({ definition, root, getFilter, getTheme }) {
  this._graphs = {};
  this._enabled = new Set();
  this._definition = definition || GRAPH_DEFINITIONS;
  this._root = root;
  this._getFilter = getFilter;
  this._getTheme = getTheme;
  this._primaryLink = Object.keys(this._definition).filter(name => this._definition[name].primaryLink)[0];
  this.$ = root.ownerDocument.querySelector.bind(root.ownerDocument);

  EventEmitter.decorate(this);
  this._onSelecting = this._onSelecting.bind(this);
}

GraphsController.prototype = {

  


  get: function (graphName) {
    return this._graphs[graphName];
  },

  






  render: Task.async(function *(recordingData, resolution) {
    
    
    
    
    yield (this._rendering && this._rendering.promise);

    
    
    if (this._destroyed) {
      return;
    }

    this._rendering = promise.defer();
    for (let graph of (yield this._getEnabled())) {
      yield graph.setPerformanceData(recordingData, resolution);
      this.emit("rendered", graph.graphName);
    }
    this._rendering.resolve();
  }),

  


  destroy: Task.async(function *() {
    let primary = this._getPrimaryLink();

    this._destroyed = true;

    if (primary) {
      primary.off("selecting", this._onSelecting);
    }

    
    
    if (this._rendering) {
      yield this._rendering.promise;
    }

    for (let graph of this.getWidgets()) {
      yield graph.destroy();
    }
  }),

  



  setTheme: function (options={}) {
    let theme = options.theme || this._getTheme();
    for (let graph of this.getWidgets()) {
      graph.setTheme(theme);
      graph.refresh({ force: options.redraw });
    }
  },

  




  isAvailable: Task.async(function *(graphName) {
    if (!this._enabled.has(graphName)) {
      return null;
    }

    let graph = this.get(graphName);

    if (!graph) {
      graph = yield this._construct(graphName);
    }

    yield graph.ready();
    return graph;
  }),

  



  enable: function (graphName, isEnabled) {
    let el = this.$(this._definition[graphName].selector);
    el.hidden = !isEnabled;

    
    if (this._enabled.has(graphName) === isEnabled) {
      return;
    }
    if (isEnabled) {
      this._enabled.add(graphName);
    } else {
      this._enabled.delete(graphName);
    }

    
    this._enabledGraphs = null;
  },

  




  disableAll: function () {
    this._root.hidden = true;
    
    Object.keys(this._definition).forEach(graphName => this.enable(graphName, false));
  },

  



  setMappedSelection: function (selection, { mapStart, mapEnd }) {
    return this._getPrimaryLink().setMappedSelection(selection, { mapStart, mapEnd });
  },

  



  getMappedSelection: function ({ mapStart, mapEnd }) {
    let primary = this._getPrimaryLink();
    if (primary && primary.hasData()) {
      return primary.getMappedSelection({ mapStart, mapEnd });
    } else {
      return null;
    }
  },

  



  getWidgets: function () {
    return Object.keys(this._graphs).map(name => this._graphs[name]);
  },

  


  dropSelection: function () {
    if (this._getPrimaryLink()) {
      return this._getPrimaryLink().dropSelection();
    }
  },

  


  selectionEnabled: Task.async(function *(enabled) {
    for (let graph of (yield this._getEnabled())) {
      graph.selectionEnabled = enabled;
    }
  }),

  


  _construct: Task.async(function *(graphName) {
    let def = this._definition[graphName];
    let el = this.$(def.selector);
    let filter = this._getFilter();
    let graph = this._graphs[graphName] = new def.constructor(el, filter);
    graph.graphName = graphName;

    yield graph.ready();

    
    if (def.primaryLink) {
      graph.on("selecting", this._onSelecting);
    } else {
      CanvasGraphUtils.linkAnimation(this._getPrimaryLink(), graph);
      CanvasGraphUtils.linkSelection(this._getPrimaryLink(), graph);
    }

    this.setTheme();
    return graph;
  }),

  



  _getPrimaryLink: function () {
    return this.get(this._primaryLink);
  },

  


  _onSelecting: function () {
    this.emit("selecting");
  },

  





  _getEnabled: Task.async(function *() {
    if (this._enabledGraphs) {
      return this._enabledGraphs;
    }
    let enabled = [];
    for (let graphName of this._enabled) {
      let graph;
      if (graph = yield this.isAvailable(graphName)) {
        enabled.push(graph);
      }
    }
    return this._enabledGraphs = enabled;
  }),
};

exports.FramerateGraph = FramerateGraph;
exports.MemoryGraph = MemoryGraph;
exports.TimelineGraph = TimelineGraph;
exports.GraphsController = GraphsController;
