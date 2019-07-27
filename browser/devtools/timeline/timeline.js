


"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/devtools/Loader.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");
Cu.import("resource:///modules/devtools/gDevTools.jsm");

devtools.lazyRequireGetter(this, "promise");
devtools.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");

devtools.lazyRequireGetter(this, "MarkersOverview",
  "devtools/shared/timeline/markers-overview", true);
devtools.lazyRequireGetter(this, "MemoryOverview",
  "devtools/shared/timeline/memory-overview", true);
devtools.lazyRequireGetter(this, "Waterfall",
  "devtools/shared/timeline/waterfall", true);
devtools.lazyRequireGetter(this, "MarkerDetails",
  "devtools/shared/timeline/marker-details", true);
devtools.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/shared/timeline/global", true);

devtools.lazyImporter(this, "CanvasGraphUtils",
  "resource:///modules/devtools/Graphs.jsm");

devtools.lazyImporter(this, "PluralForm",
  "resource://gre/modules/PluralForm.jsm");

const OVERVIEW_UPDATE_INTERVAL = 200;
const OVERVIEW_INITIAL_SELECTION_RATIO = 0.15;





const Prefs = new ViewHelpers.Prefs("devtools.timeline", {
  hiddenMarkers: ["Json", "hiddenMarkers"]
});


const EVENTS = {
  
  RECORDING_STARTED: "Timeline:RecordingStarted",
  RECORDING_ENDED: "Timeline:RecordingEnded",

  
  OVERVIEW_UPDATED: "Timeline:OverviewUpdated",

  
  WATERFALL_UPDATED: "Timeline:WaterfallUpdated"
};




let gToolbox, gTarget, gFront;




let startupTimeline = Task.async(function*() {
  yield TimelineView.initialize();
  yield TimelineController.initialize();
});




let shutdownTimeline = Task.async(function*() {
  yield TimelineView.destroy();
  yield TimelineController.destroy();
  yield gFront.stop();
});




let TimelineController = {
  



  _starTime: 0,
  _endTime: 0,
  _markers: [],
  _memory: [],
  _frames: [],

  


  initialize: function() {
    this._onRecordingTick = this._onRecordingTick.bind(this);
    this._onMarkers = this._onMarkers.bind(this);
    this._onMemory = this._onMemory.bind(this);
    this._onFrames = this._onFrames.bind(this);

    gFront.on("markers", this._onMarkers);
    gFront.on("memory", this._onMemory);
    gFront.on("frames", this._onFrames);
  },

  


  destroy: function() {
    gFront.off("markers", this._onMarkers);
    gFront.off("memory", this._onMemory);
    gFront.off("frames", this._onFrames);
  },

  



  getInterval: function() {
    return { startTime: this._startTime, endTime: this._endTime };
  },

  



  getMarkers: function() {
    return this._markers;
  },

  



  getMemory: function() {
    return this._memory;
  },

  





  getFrames: function() {
    return this._frames;
  },

  


  updateMemoryRecording: Task.async(function*() {
    if ($("#memory-checkbox").checked) {
      yield TimelineView.showMemoryOverview();
    } else {
      yield TimelineView.hideMemoryOverview();
    }
  }),

  


  toggleRecording: Task.async(function*() {
    let isRecording = yield gFront.isRecording();
    if (isRecording == false) {
      yield this._startRecording();
    } else {
      yield this._stopRecording();
    }
  }),

  


  _startRecording: function*() {
    TimelineView.handleRecordingStarted();

    let withMemory = $("#memory-checkbox").checked;
    let startTime = yield gFront.start({ withMemory });

    
    
    
    
    
    this._localStartTime = performance.now();
    this._startTime = startTime;
    this._endTime = startTime;
    this._markers = [];
    this._memory = [];
    this._frames = [];
    this._updateId = setInterval(this._onRecordingTick, OVERVIEW_UPDATE_INTERVAL);
  },

  


  _stopRecording: function*() {
    clearInterval(this._updateId);

    
    this._markers = this._markers.sort((a,b) => (a.start > b.start));

    TimelineView.handleRecordingUpdate();
    TimelineView.handleRecordingEnded();
    yield gFront.stop();
  },

  



  _stopRecordingAndDiscardData: function*() {
    
    
    this._markers.length = 0;
    this._memory.length = 0;

    yield this._stopRecording();

    
    
    
    
    this._markers.length = 0;
    this._memory.length = 0;
  },

  








  _onMarkers: function(markers, endTime) {
    for (let marker of markers) {
      marker.start -= this._startTime;
      marker.end -= this._startTime;
    }
    Array.prototype.push.apply(this._markers, markers);
    this._endTime = endTime;
  },

  







  _onMemory: function(delta, measurement) {
    this._memory.push({
      delta: delta - this._startTime,
      value: measurement.total / 1024 / 1024
    });
  },

  







  _onFrames: function(delta, frames) {
    Array.prototype.push.apply(this._frames, frames);
  },

  



  _onRecordingTick: function() {
    
    
    
    let fakeTime = this._startTime + (performance.now() - this._localStartTime);
    if (fakeTime > this._endTime) {
      this._endTime = fakeTime;
    }
    TimelineView.handleRecordingUpdate();
  }
};




let TimelineView = {
  


  initialize: Task.async(function*() {
    let blueprint = this._getFilteredBluePrint();
    this.markersOverview = new MarkersOverview($("#markers-overview"), blueprint);
    this.waterfall = new Waterfall($("#timeline-waterfall"), $("#timeline-pane"), blueprint);
    this.markerDetails = new MarkerDetails($("#timeline-waterfall-details"), $("#timeline-waterfall-container > splitter"));

    this._onThemeChange = this._onThemeChange.bind(this);
    this._onSelecting = this._onSelecting.bind(this);
    this._onRefresh = this._onRefresh.bind(this);

    gDevTools.on("pref-changed", this._onThemeChange);
    this.markersOverview.on("selecting", this._onSelecting);
    this.markersOverview.on("refresh", this._onRefresh);
    this.markerDetails.on("resize", this._onRefresh);

    this._onMarkerSelected = this._onMarkerSelected.bind(this);
    this.waterfall.on("selected", this._onMarkerSelected);
    this.waterfall.on("unselected", this._onMarkerSelected);

    let theme = Services.prefs.getCharPref("devtools.theme");
    this.markersOverview.setTheme(theme);

    yield this.markersOverview.ready();

    yield this.waterfall.recalculateBounds();

    this._buildFilterPopup();
  }),

  


  destroy: function() {
    gDevTools.off("pref-changed", this._onThemeChange);
    this.markerDetails.off("resize", this._onRefresh);
    this.markerDetails.destroy();
    this.waterfall.off("selected", this._onMarkerSelected);
    this.waterfall.off("unselected", this._onMarkerSelected);
    this.waterfall.destroy();
    this.markersOverview.off("selecting", this._onSelecting);
    this.markersOverview.off("refresh", this._onRefresh);
    this.markersOverview.destroy();

    
    if (this.memoryOverview) {
      this.memoryOverview.destroy();
    }
  },

  


  showMemoryOverview: Task.async(function*() {
    let theme = Services.prefs.getCharPref("devtools.theme");

    this.memoryOverview = new MemoryOverview($("#memory-overview"));
    this.memoryOverview.setTheme(theme);
    yield this.memoryOverview.ready();

    let memory = TimelineController.getMemory();
    this.memoryOverview.setData(memory);

    CanvasGraphUtils.linkAnimation(this.markersOverview, this.memoryOverview);
    CanvasGraphUtils.linkSelection(this.markersOverview, this.memoryOverview);
  }),

  


  hideMemoryOverview: function() {
    if (!this.memoryOverview) {
      return;
    }
    this.memoryOverview.destroy();
    this.memoryOverview = null;
  },

  


  _onMarkerSelected: function(event, marker) {
    if (event == "selected") {
      this.markerDetails.render({
        toolbox: gToolbox,
        marker: marker,
        frames: TimelineController.getFrames()
      });
    }
    if (event == "unselected") {
      this.markerDetails.empty();
    }
  },

  



  handleRecordingStarted: function() {
    $("#record-button").setAttribute("checked", "true");
    $("#memory-checkbox").setAttribute("disabled", "true");
    $("#timeline-pane").selectedPanel = $("#recording-notice");

    this.markersOverview.clearView();

    
    if (this.memoryOverview) {
      this.memoryOverview.clearView();
    }

    this.waterfall.clearView();

    window.emit(EVENTS.RECORDING_STARTED);
  },

  



  handleRecordingEnded: function() {
    $("#record-button").removeAttribute("checked");
    $("#memory-checkbox").removeAttribute("disabled");
    $("#timeline-pane").selectedPanel = $("#timeline-waterfall-container");

    this.markersOverview.selectionEnabled = true;

    
    if (this.memoryOverview) {
      this.memoryOverview.selectionEnabled = true;
    }

    let interval = TimelineController.getInterval();
    let markers = TimelineController.getMarkers();
    let memory = TimelineController.getMemory();

    if (markers.length) {
      let start = markers[0].start * this.markersOverview.dataScaleX;
      let end = start + this.markersOverview.width * OVERVIEW_INITIAL_SELECTION_RATIO;
      this.markersOverview.setSelection({ start, end });
    } else {
      let startTime = interval.startTime;
      let endTime = interval.endTime;
      this.waterfall.setData({ markers, interval: { startTime, endTime } });
    }

    window.emit(EVENTS.RECORDING_ENDED);
  },

  



  handleRecordingUpdate: function() {
    let interval = TimelineController.getInterval();
    let markers = TimelineController.getMarkers();
    let memory = TimelineController.getMemory();

    let duration = interval.endTime - interval.startTime;
    this.markersOverview.setData({ markers, duration });

    
    if (this.memoryOverview) {
      this.memoryOverview.setData(memory);
    }

    window.emit(EVENTS.OVERVIEW_UPDATED);
  },

  


  _onSelecting: function() {
    if (!this.markersOverview.hasSelection() &&
        !this.markersOverview.hasSelectionInProgress()) {
      this.waterfall.clearView();
      return;
    }
    this.waterfall.resetSelection();
    this.updateWaterfall();
  },

  


  updateWaterfall: function() {
    let selection = this.markersOverview.getSelection();
    let start = selection.start / this.markersOverview.dataScaleX;
    let end = selection.end / this.markersOverview.dataScaleX;

    let markers = TimelineController.getMarkers();
    let interval = TimelineController.getInterval();

    let startTime = Math.min(start, end);
    let endTime = Math.max(start, end);

    this.waterfall.setData({ markers, interval: { startTime, endTime } });
  },

  


  _onRefresh: function() {
    this.waterfall.recalculateBounds();
    this.updateWaterfall();
  },

  


  _getFilteredBluePrint: function() {
    let hiddenMarkers = Prefs.hiddenMarkers;
    let filteredBlueprint = Cu.cloneInto(TIMELINE_BLUEPRINT, {});
    let maybeRemovedGroups = new Set();
    let removedGroups = new Set();

    

    for (let hiddenMarkerName of hiddenMarkers) {
      maybeRemovedGroups.add(filteredBlueprint[hiddenMarkerName].group);
      delete filteredBlueprint[hiddenMarkerName];
    }

    

    for (let removedGroup of maybeRemovedGroups) {
      let markerNames = Object.keys(filteredBlueprint);
      let allGroupsRemoved = markerNames.every(e => filteredBlueprint[e].group != removedGroup);
      if (allGroupsRemoved) {
        removedGroups.add(removedGroup);
      }
    }

    

    for (let removedGroup of removedGroups) {
      for (let [, markerDetails] of Iterator(filteredBlueprint)) {
        if (markerDetails.group > removedGroup) {
          markerDetails.group--;
        }
      }
    }

    return filteredBlueprint;

  },

  



  _onHiddenMarkersChanged: function(e) {
    let menuItems = $$("#timelineFilterPopup menuitem[marker-type]:not([checked])");
    let hiddenMarkers = Array.map(menuItems, e => e.getAttribute("marker-type"));

    Prefs.hiddenMarkers = hiddenMarkers;
    let blueprint = this._getFilteredBluePrint();

    this.waterfall.setBlueprint(blueprint);
    this.updateWaterfall();

    this.markersOverview.setBlueprint(blueprint);
    this.markersOverview.refresh({ force: true });
  },

  


  _buildFilterPopup: function() {
    let popup = $("#timelineFilterPopup");
    let button = $("#filter-button");

    popup.addEventListener("popupshowing", () => button.setAttribute("open", "true"));
    popup.addEventListener("popuphiding",  () => button.removeAttribute("open"));

    this._onHiddenMarkersChanged = this._onHiddenMarkersChanged.bind(this);

    for (let [markerName, markerDetails] of Iterator(TIMELINE_BLUEPRINT)) {
      let menuitem = document.createElement("menuitem");
      menuitem.setAttribute("closemenu", "none");
      menuitem.setAttribute("type", "checkbox");
      menuitem.setAttribute("marker-type", markerName);
      menuitem.setAttribute("label", markerDetails.label);
      menuitem.setAttribute("flex", "1");
      menuitem.setAttribute("align", "center");

      menuitem.addEventListener("command", this._onHiddenMarkersChanged);

      if (Prefs.hiddenMarkers.indexOf(markerName) == -1) {
        menuitem.setAttribute("checked", "true");
      }

      
      let bulletStyle = `--bullet-bg: ${markerDetails.fill};`
      bulletStyle += `--bullet-border: ${markerDetails.stroke}`;
      menuitem.setAttribute("style", bulletStyle);

      popup.appendChild(menuitem);
    }
  },

  



  _onThemeChange: function (_, theme) {
    if (this.memoryOverview) {
      this.memoryOverview.setTheme(theme.newValue);
      this.memoryOverview.refresh({ force: true });
    }

    this.markersOverview.setTheme(theme.newValue);
    this.markersOverview.refresh({ force: true });
  }
};




EventEmitter.decorate(this);




function $(selector, target = document) {
  return target.querySelector(selector);
}
function $$(selector, target = document) {
  return target.querySelectorAll(selector);
}
