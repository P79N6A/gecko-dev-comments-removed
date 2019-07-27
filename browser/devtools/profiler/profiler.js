


"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/devtools/Loader.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

devtools.lazyRequireGetter(this, "Services");
devtools.lazyRequireGetter(this, "promise");
devtools.lazyRequireGetter(this, "EventEmitter",
  "devtools/toolkit/event-emitter");
devtools.lazyRequireGetter(this, "DevToolsUtils",
  "devtools/toolkit/DevToolsUtils");
devtools.lazyRequireGetter(this, "FramerateFront",
  "devtools/server/actors/framerate", true);

devtools.lazyRequireGetter(this, "L10N",
  "devtools/profiler/global", true);
devtools.lazyRequireGetter(this, "CATEGORIES",
  "devtools/profiler/global", true);
devtools.lazyRequireGetter(this, "CATEGORY_MAPPINGS",
  "devtools/profiler/global", true);
devtools.lazyRequireGetter(this, "CATEGORY_OTHER",
  "devtools/profiler/global", true);
devtools.lazyRequireGetter(this, "ThreadNode",
  "devtools/profiler/tree-model", true);
devtools.lazyRequireGetter(this, "CallView",
  "devtools/profiler/tree-view", true);

devtools.lazyImporter(this, "FileUtils",
  "resource://gre/modules/FileUtils.jsm");
devtools.lazyImporter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");
devtools.lazyImporter(this, "LineGraphWidget",
  "resource:///modules/devtools/Graphs.jsm");
devtools.lazyImporter(this, "BarGraphWidget",
  "resource:///modules/devtools/Graphs.jsm");
devtools.lazyImporter(this, "CanvasGraphUtils",
  "resource:///modules/devtools/Graphs.jsm");
devtools.lazyImporter(this, "SideMenuWidget",
  "resource:///modules/devtools/SideMenuWidget.jsm");

const RECORDING_DATA_DISPLAY_DELAY = 10; 
const FRAMERATE_CALC_INTERVAL = 16; 
const FRAMERATE_GRAPH_HEIGHT = 60; 
const CATEGORIES_GRAPH_HEIGHT = 60; 
const CATEGORIES_GRAPH_MIN_BARS_WIDTH = 3; 
const CALL_VIEW_FOCUS_EVENTS_DRAIN = 10; 
const GRAPH_SCROLL_EVENTS_DRAIN = 50; 
const GRAPH_ZOOM_MIN_TIMESPAN = 20; 





const PROFILE_SERIALIZER_IDENTIFIER = "Recorded Performance Data";
const PROFILE_SERIALIZER_VERSION = 1;


const EVENTS = {
  
  
  RECORDING_STARTED: "Profiler:RecordingStarted",
  RECORDING_ENDED: "Profiler:RecordingEnded",

  
  
  
  RECORDING_LOST: "Profiler:RecordingCancelled",

  
  RECORDING_DISPLAYED: "Profiler:RecordingDisplayed",

  
  TAB_SPAWNED_FROM_SELECTION: "Profiler:TabSpawnedFromSelection",

  
  TAB_SPAWNED_FROM_FRAME_NODE: "Profiler:TabSpawnedFromFrameNode",

  
  EMPTY_NOTICE_SHOWN: "Profiler:EmptyNoticeShown",
  RECORDING_NOTICE_SHOWN: "Profiler:RecordingNoticeShown",
  LOADING_NOTICE_SHOWN: "Profiler:LoadingNoticeShown",
  TABBED_BROWSER_SHOWN: "Profiler:TabbedBrowserShown",

  
  SOURCE_SHOWN_IN_JS_DEBUGGER: "Profiler:SourceShownInJsDebugger",
  SOURCE_NOT_FOUND_IN_JS_DEBUGGER: "Profiler:SourceNotFoundInJsDebugger"
};




let gToolbox, gTarget, gFront;




let startupProfiler = Task.async(function*() {
  yield promise.all([
    PrefObserver.register(),
    EventsHandler.initialize(),
    RecordingsListView.initialize(),
    ProfileView.initialize()
  ]);

  
  
  for (let recordingData of gFront.finishedConsoleRecordings) {
    let profileLabel = recordingData.profilerData.profileLabel;
    let recordingItem = RecordingsListView.addEmptyRecording(profileLabel);
    RecordingsListView.customizeRecording(recordingItem, recordingData);
  }
  for (let { profileLabel } of gFront.pendingConsoleRecordings) {
    RecordingsListView.handleRecordingStarted(profileLabel);
  }

  
  RecordingsListView.selectedIndex = 0;
});




let shutdownProfiler = Task.async(function*() {
  yield promise.all([
    PrefObserver.unregister(),
    EventsHandler.destroy(),
    RecordingsListView.destroy(),
    ProfileView.destroy()
  ]);
});





let PrefObserver = {
  register: function() {
    this.branch = Services.prefs.getBranch("devtools.profiler.");
    this.branch.addObserver("", this, false);
  },
  unregister: function() {
    this.branch.removeObserver("", this);
  },
  observe: function(subject, topic, pref) {
    Prefs.refresh();

    if (pref == "ui.show-platform-data") {
      RecordingsListView.forceSelect(RecordingsListView.selectedItem);
    }
  }
};




let EventsHandler = {
  


  initialize: function() {
    this._onConsoleProfileStart = this._onConsoleProfileStart.bind(this);
    this._onConsoleProfileEnd = this._onConsoleProfileEnd.bind(this);

    gFront.on("profile", this._onConsoleProfileStart);
    gFront.on("profileEnd", this._onConsoleProfileEnd);
    gFront.on("profiler-unexpectedly-stopped", this._onProfilerDeactivated);
  },

  


  destroy: function() {
    gFront.off("profile", this._onConsoleProfileStart);
    gFront.off("profileEnd", this._onConsoleProfileEnd);
    gFront.off("profiler-unexpectedly-stopped", this._onProfilerDeactivated);
  },

  





  _onConsoleProfileStart: function(event, profileLabel) {
    RecordingsListView.handleRecordingStarted(profileLabel);
  },

  





  _onConsoleProfileEnd: function(event, recordingData) {
    RecordingsListView.handleRecordingEnded(recordingData);
  },

  



  _onProfilerDeactivated: function() {
    RecordingsListView.removeForPredicate(e => e.isRecording);
    RecordingsListView.handleRecordingCancelled();
  }
};




const Prefs = new ViewHelpers.Prefs("devtools.profiler", {
  showPlatformData: ["Bool", "ui.show-platform-data"]
});




EventEmitter.decorate(this);




function $(selector, target = document) {
  return target.querySelector(selector);
}
function $$(selector, target = document) {
  return target.querySelectorAll(selector);
}
