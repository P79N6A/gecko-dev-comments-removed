


"use strict";




const OVERVIEW_UPDATE_INTERVAL = 200; 
const FRAMERATE_GRAPH_LOW_RES_INTERVAL = 100; 
const FRAMERATE_GRAPH_HIGH_RES_INTERVAL = 16; 
const GRAPH_REQUIREMENTS = {
  timeline: {
    actors: ["timeline"],
    features: ["withMarkers"]
  },
  framerate: {
    actors: ["timeline"],
    features: ["withTicks"]
  },
  memory: {
    actors: ["memory"],
    features: ["withMemory"]
  },
}





let OverviewView = {

  


  initialize: function () {
    this.graphs = new GraphsController({
      root: $("#overview-pane"),
      getBlueprint: () => PerformanceController.getTimelineBlueprint(),
      getTheme: () => PerformanceController.getTheme(),
    });

    
    if (!gFront.getActorSupport().timeline) {
      this.disable();
      return;
    }

    this._onRecordingWillStart = this._onRecordingWillStart.bind(this);
    this._onRecordingStarted = this._onRecordingStarted.bind(this);
    this._onRecordingWillStop = this._onRecordingWillStop.bind(this);
    this._onRecordingStopped = this._onRecordingStopped.bind(this);
    this._onRecordingSelected = this._onRecordingSelected.bind(this);
    this._onRecordingTick = this._onRecordingTick.bind(this);
    this._onGraphSelecting = this._onGraphSelecting.bind(this);
    this._onGraphRendered = this._onGraphRendered.bind(this);
    this._onPrefChanged = this._onPrefChanged.bind(this);
    this._onThemeChanged = this._onThemeChanged.bind(this);

    
    
    PerformanceController.on(EVENTS.PREF_CHANGED, this._onPrefChanged);
    PerformanceController.on(EVENTS.THEME_CHANGED, this._onThemeChanged);
    PerformanceController.on(EVENTS.RECORDING_WILL_START, this._onRecordingWillStart);
    PerformanceController.on(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.on(EVENTS.RECORDING_WILL_STOP, this._onRecordingWillStop);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.on(EVENTS.RECORDING_SELECTED, this._onRecordingSelected);
    PerformanceController.on(EVENTS.CONSOLE_RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.on(EVENTS.CONSOLE_RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.on(EVENTS.CONSOLE_RECORDING_WILL_STOP, this._onRecordingWillStop);
    this.graphs.on("selecting", this._onGraphSelecting);
    this.graphs.on("rendered", this._onGraphRendered);
  },

  


  destroy: Task.async(function*() {
    PerformanceController.off(EVENTS.PREF_CHANGED, this._onPrefChanged);
    PerformanceController.off(EVENTS.THEME_CHANGED, this._onThemeChanged);
    PerformanceController.off(EVENTS.RECORDING_WILL_START, this._onRecordingWillStart);
    PerformanceController.off(EVENTS.RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.off(EVENTS.RECORDING_WILL_STOP, this._onRecordingWillStop);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.off(EVENTS.RECORDING_SELECTED, this._onRecordingSelected);
    PerformanceController.off(EVENTS.CONSOLE_RECORDING_STARTED, this._onRecordingStarted);
    PerformanceController.off(EVENTS.CONSOLE_RECORDING_STOPPED, this._onRecordingStopped);
    PerformanceController.off(EVENTS.CONSOLE_RECORDING_WILL_STOP, this._onRecordingWillStop);
    this.graphs.off("selecting", this._onGraphSelecting);
    this.graphs.off("rendered", this._onGraphRendered);
    yield this.graphs.destroy();
  }),

  



  get isMouseActive() {
    
    
    
    return !!this.graphs.getWidgets().some(e => e.isMouseActive);
  },

  




  disable: function () {
    this._disabled = true;
    this.graphs.disableAll();
  },

  




  isDisabled: function () {
    return this._disabled;
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
    this.graphs.setMappedSelection(selection, { mapStart, mapEnd });
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
    let selection = this.graphs.getMappedSelection({ mapStart, mapEnd });
    return { startTime: selection.min, endTime: selection.max };
  },

  





  render: Task.async(function *(resolution) {
    if (this.isDisabled()) {
      return;
    }
    let recording = PerformanceController.getCurrentRecording();
    yield this.graphs.render(recording.getAllData(), resolution);

    
    this.emit(EVENTS.OVERVIEW_RENDERED, resolution);
  }),

  




  _onRecordingTick: Task.async(function *() {
    yield this.render(FRAMERATE_GRAPH_LOW_RES_INTERVAL);
    this._prepareNextTick();
  }),

  


  _prepareNextTick: function () {
    
    
    if (this.isRendering()) {
      this._timeoutId = setTimeout(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);
    }
  },

  




  _onRecordingWillStart: Task.async(function* () {
    this._onRecordingStateChange();
    yield this._checkSelection();
    this.graphs.dropSelection();
  }),

  


  _onRecordingStarted: function (_, recording) {
    this._onRecordingStateChange();
  },

  


  _onRecordingWillStop: function(_, recording) {
    this._onRecordingStateChange();
  },

  


  _onRecordingStopped: Task.async(function* (_, recording) {
    this._onRecordingStateChange();
    
    
    
    
    
    if (recording !== PerformanceController.getCurrentRecording()) {
      return;
    }
    this.render(FRAMERATE_GRAPH_HIGH_RES_INTERVAL);
    yield this._checkSelection(recording);
  }),

  


  _onRecordingSelected: Task.async(function* (_, recording) {
    if (!recording) {
      return;
    }
    this._onRecordingStateChange();
    this._setGraphVisibilityFromRecordingFeatures(recording);

    
    if (!recording.isRecording()) {
      yield this.render(FRAMERATE_GRAPH_HIGH_RES_INTERVAL);
    }
    yield this._checkSelection(recording);
    this.graphs.dropSelection();
  }),

  




  _onRecordingStateChange: function () {
    let currentRecording = PerformanceController.getCurrentRecording();
    if (!currentRecording || (this.isRendering() && !currentRecording.isRecording())) {
      this._stopPolling();
    } else if (currentRecording.isRecording() && !this.isRendering()) {
      this._startPolling();
    }
  },

  


  _startPolling: function () {
    this._timeoutId = setTimeout(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);
  },

  


  _stopPolling: function () {
    clearTimeout(this._timeoutId);
    this._timeoutId = null;
  },

  


  isRendering: function () {
    return !!this._timeoutId;
  },

  



  _checkSelection: Task.async(function* (recording) {
    let isEnabled = recording ? !recording.isRecording() : false;
    yield this.graphs.selectionEnabled(isEnabled);
  }),

  



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

  _onGraphRendered: function (_, graphName) {
    switch (graphName) {
      case "timeline":
        this.emit(EVENTS.MARKERS_GRAPH_RENDERED);
        break;
      case "memory":
        this.emit(EVENTS.MEMORY_GRAPH_RENDERED);
        break;
      case "framerate":
        this.emit(EVENTS.FRAMERATE_GRAPH_RENDERED);
        break;
    }
  },

  





  _onPrefChanged: Task.async(function* (_, prefName, prefValue) {
    switch (prefName) {
      case "hidden-markers": {
        let graph;
        if (graph = yield this.graphs.isAvailable("timeline")) {
          let blueprint = PerformanceController.getTimelineBlueprint();
          graph.setBlueprint(blueprint);
          graph.refresh({ force: true });
        }
        break;
      }
    }
  }),

  _setGraphVisibilityFromRecordingFeatures: function (recording) {
    for (let [graphName, requirements] of Iterator(GRAPH_REQUIREMENTS)) {
      this.graphs.enable(graphName, PerformanceController.isFeatureSupported(requirements));
    }
  },

  


  _onThemeChanged: function (_, theme) {
    this.graphs.setTheme({ theme, redraw: true });
  },

  toString: () => "[object OverviewView]"
};


EventEmitter.decorate(OverviewView);
