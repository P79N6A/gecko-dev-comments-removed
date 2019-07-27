


"use strict";




const OVERVIEW_UPDATE_INTERVAL = 200; 

const FRAMERATE_GRAPH_LOW_RES_INTERVAL = 100; 
const FRAMERATE_GRAPH_HIGH_RES_INTERVAL = 16; 

const MARKERS_GRAPH_HEADER_HEIGHT = 14; 
const MARKERS_GRAPH_ROW_HEIGHT = 10; 
const MARKERS_GROUP_VERTICAL_PADDING = 4; 





let OverviewView = {
  


  initialize: function () {
    if (gFront.getMocksInUse().timeline) {
      this.disable();
    }
    this._onRecordingWillStart = this._onRecordingWillStart.bind(this);
    this._onRecordingStarted = this._onRecordingStarted.bind(this);
    this._onRecordingWillStop = this._onRecordingWillStop.bind(this);
    this._onRecordingStopped = this._onRecordingStopped.bind(this);
    this._onRecordingSelected = this._onRecordingSelected.bind(this);
    this._onRecordingTick = this._onRecordingTick.bind(this);
    this._onGraphSelecting = this._onGraphSelecting.bind(this);
    this._onPrefChanged = this._onPrefChanged.bind(this);
    this._onThemeChanged = this._onThemeChanged.bind(this);

    
    
    $("#memory-overview").hidden = !PerformanceController.getOption("enable-memory");
    $("#time-framerate").hidden = !PerformanceController.getOption("enable-framerate");

    PerformanceController.on(EVENTS.PREF_CHANGED, this._onPrefChanged);
    PerformanceController.on(EVENTS.THEME_CHANGED, this._onThemeChanged);
    PerformanceController.on(EVENTS.RECORDING_WILL_START, this._onRecordingWillStart);
    PerformanceController.on(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.on(EVENTS.RECORDING_WILL_STOP, this._onRecordingWillStop);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.on(EVENTS.RECORDING_SELECTED, this._onRecordingSelected);
  },

  


  destroy: Task.async(function*() {
    if (this.markersOverview) {
      yield this.markersOverview.destroy();
    }
    if (this.memoryOverview) {
      yield this.memoryOverview.destroy();
    }
    if (this.framerateGraph) {
      yield this.framerateGraph.destroy();
    }

    PerformanceController.off(EVENTS.PREF_CHANGED, this._onPrefChanged);
    PerformanceController.off(EVENTS.THEME_CHANGED, this._onThemeChanged);
    PerformanceController.off(EVENTS.RECORDING_WILL_START, this._onRecordingWillStart);
    PerformanceController.off(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.off(EVENTS.RECORDING_WILL_STOP, this._onRecordingWillStop);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.off(EVENTS.RECORDING_SELECTED, this._onRecordingSelected);
  }),

  




  disable: function () {
    this._disabled = true;
    $("#overview-pane").hidden = true;
  },

  




  isDisabled: function () {
    return this._disabled;
  },

  


  setTheme: function (options={}) {
    let theme = options.theme || PerformanceController.getTheme();

    if (this.framerateGraph) {
      this.framerateGraph.setTheme(theme);
      this.framerateGraph.refresh({ force: options.redraw });
    }

    if (this.markersOverview) {
      this.markersOverview.setTheme(theme);
      this.markersOverview.refresh({ force: options.redraw });
    }

    if (this.memoryOverview) {
      this.memoryOverview.setTheme(theme);
      this.memoryOverview.refresh({ force: options.redraw });
    }
  },

  





  setTimeInterval: function(interval, options = {}) {
    let recording = PerformanceController.getCurrentRecording();
    if (recording == null) {
      throw new Error("A recording should be available in order to set the selection.");
    }
    if (this.isDisabled()) {
      return;
    }
    let mapStart = () => 0;
    let mapEnd = () => recording.getDuration();
    let selection = { start: interval.startTime, end: interval.endTime };
    this._stopSelectionChangeEventPropagation = options.stopPropagation;
    this.markersOverview.setMappedSelection(selection, { mapStart, mapEnd });
    this._stopSelectionChangeEventPropagation = false;
  },

  





  getTimeInterval: function() {
    let recording = PerformanceController.getCurrentRecording();
    if (recording == null) {
      throw new Error("A recording should be available in order to get the selection.");
    }
    if (this.isDisabled()) {
      return { startTime: 0, endTime: recording.getDuration() };
    }
    let mapStart = () => 0;
    let mapEnd = () => recording.getDuration();
    let selection = this.markersOverview.getMappedSelection({ mapStart, mapEnd });
    return { startTime: selection.min, endTime: selection.max };
  },

  





  _markersGraphAvailable: Task.async(function *() {
    if (this.markersOverview) {
      yield this.markersOverview.ready();
      return true;
    }
    let blueprint = PerformanceController.getTimelineBlueprint();
    this.markersOverview = new MarkersOverview($("#markers-overview"), blueprint);
    this.markersOverview.headerHeight = MARKERS_GRAPH_HEADER_HEIGHT;
    this.markersOverview.rowHeight = MARKERS_GRAPH_ROW_HEIGHT;
    this.markersOverview.groupPadding = MARKERS_GROUP_VERTICAL_PADDING;
    this.markersOverview.on("selecting", this._onGraphSelecting);
    yield this.markersOverview.ready();
    this.setTheme();
    return true;
  }),

  






  _memoryGraphAvailable: Task.async(function *() {
    if (!PerformanceController.getOption("enable-memory")) {
      return false;
    }
    if (this.memoryOverview) {
      yield this.memoryOverview.ready();
      return true;
    }
    this.memoryOverview = new MemoryGraph($("#memory-overview"));
    yield this.memoryOverview.ready();
    this.setTheme();

    CanvasGraphUtils.linkAnimation(this.markersOverview, this.memoryOverview);
    CanvasGraphUtils.linkSelection(this.markersOverview, this.memoryOverview);
    return true;
  }),

  






  _framerateGraphAvailable: Task.async(function *() {
    if (!PerformanceController.getOption("enable-framerate")) {
      return false;
    }
    if (this.framerateGraph) {
      yield this.framerateGraph.ready();
      return true;
    }
    this.framerateGraph = new FramerateGraph($("#time-framerate"));
    yield this.framerateGraph.ready();
    this.setTheme();

    CanvasGraphUtils.linkAnimation(this.markersOverview, this.framerateGraph);
    CanvasGraphUtils.linkSelection(this.markersOverview, this.framerateGraph);
    return true;
  }),

  





  render: Task.async(function *(resolution) {
    if (this.isDisabled()) {
      return;
    }
    let recording = PerformanceController.getCurrentRecording();
    let duration = recording.getDuration();
    let markers = recording.getMarkers();
    let memory = recording.getMemory();
    let timestamps = recording.getTicks();

    
    if (markers && (yield this._markersGraphAvailable())) {
      this.markersOverview.setData({ markers, duration });
      this.emit(EVENTS.MARKERS_GRAPH_RENDERED);
    }
    if (memory && (yield this._memoryGraphAvailable())) {
      this.memoryOverview.dataDuration = duration;
      this.memoryOverview.setData(memory);
      this.emit(EVENTS.MEMORY_GRAPH_RENDERED);
    }
    if (timestamps && (yield this._framerateGraphAvailable())) {
      this.framerateGraph.dataDuration = duration;
      yield this.framerateGraph.setDataFromTimestamps(timestamps, resolution);
      this.emit(EVENTS.FRAMERATE_GRAPH_RENDERED);
    }

    
    this.emit(EVENTS.OVERVIEW_RENDERED);
  }),

  




  _onRecordingTick: Task.async(function *() {
    yield this.render(FRAMERATE_GRAPH_LOW_RES_INTERVAL);
    this._prepareNextTick();
  }),

  


  _prepareNextTick: function () {
    
    
    if (this._timeoutId) {
      this._timeoutId = setTimeout(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);
    }
  },

  



  _onGraphSelecting: function () {
    if (this._stopSelectionChangeEventPropagation) {
      return;
    }
    
    
    let interval = this.getTimeInterval();
    if (interval.endTime - interval.startTime < 1) {
      this.emit(EVENTS.OVERVIEW_RANGE_CLEARED);
    } else {
      this.emit(EVENTS.OVERVIEW_RANGE_SELECTED, interval);
    }
  },

  


  _onRecordingWillStart: Task.async(function* (_, recording) {
    yield this._checkSelection(recording);
    this.markersOverview.dropSelection();
  }),

  


  _onRecordingStarted: function (_, recording) {
    this._timeoutId = setTimeout(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);
  },

  


  _onRecordingWillStop: function(_, recording) {
    clearTimeout(this._timeoutId);
    this._timeoutId = null;
  },

  


  _onRecordingStopped: Task.async(function* (_, recording) {
    this.render(FRAMERATE_GRAPH_HIGH_RES_INTERVAL);
    yield this._checkSelection(recording);
  }),

  


  _onRecordingSelected: Task.async(function* (_, recording) {
    if (!recording) {
      return;
    }
    
    
    if (!this._timeoutId) {
      yield this.render(FRAMERATE_GRAPH_HIGH_RES_INTERVAL);
    }
    yield this._checkSelection(recording);
    this.markersOverview.dropSelection();
  }),

  



  _checkSelection: Task.async(function* (recording) {
    let selectionEnabled = !recording.isRecording();

    if (yield this._markersGraphAvailable()) {
      this.markersOverview.selectionEnabled = selectionEnabled;
    }
    if (yield this._memoryGraphAvailable()) {
      this.memoryOverview.selectionEnabled = selectionEnabled;
    }
    if (yield this._framerateGraphAvailable()) {
      this.framerateGraph.selectionEnabled = selectionEnabled;
    }
  }),

  



  _onPrefChanged: Task.async(function* (_, prefName, prefValue) {
    switch (prefName) {
      case "enable-memory": {
        $("#memory-overview").hidden = !prefValue;
        break;
      }
      case "enable-framerate": {
        $("#time-framerate").hidden = !prefValue;
        break;
      }
      case "hidden-markers": {
        if (yield this._markersGraphAvailable()) {
          let blueprint = PerformanceController.getTimelineBlueprint();
          this.markersOverview.setBlueprint(blueprint);
          this.markersOverview.refresh({ force: true });
        }
        break;
      }
    }
  }),

  


  _onThemeChanged: function (_, theme) {
    this.setTheme({ theme, redraw: true });
  },

  toString: () => "[object OverviewView]"
};


EventEmitter.decorate(OverviewView);
