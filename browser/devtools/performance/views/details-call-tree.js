


"use strict";




let CallTreeView = {
  


  initialize: function () {
    this._callTree = $(".call-tree-cells-container");
    this._onRecordingStoppedOrSelected = this._onRecordingStoppedOrSelected.bind(this);
    this._onRangeChange = this._onRangeChange.bind(this);
    this._onPrefChanged = this._onPrefChanged.bind(this);
    this._onLink = this._onLink.bind(this);

    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.on(EVENTS.RECORDING_SELECTED, this._onRecordingStoppedOrSelected);
    PerformanceController.on(EVENTS.PREF_CHANGED, this._onPrefChanged);
    OverviewView.on(EVENTS.OVERVIEW_RANGE_SELECTED, this._onRangeChange);
    OverviewView.on(EVENTS.OVERVIEW_RANGE_CLEARED, this._onRangeChange);
  },

  


  destroy: function () {
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._onRecordingStoppedOrSelected);
    PerformanceController.off(EVENTS.RECORDING_SELECTED, this._onRecordingStoppedOrSelected);
    PerformanceController.off(EVENTS.PREF_CHANGED, this._onPrefChanged);
    OverviewView.off(EVENTS.OVERVIEW_RANGE_SELECTED, this._onRangeChange);
    OverviewView.off(EVENTS.OVERVIEW_RANGE_CLEARED, this._onRangeChange);
  },

  


  render: function (profilerData, beginAt, endAt, options={}) {
    
    if (profilerData.profile == null) {
      return;
    }
    let threadNode = this._prepareCallTree(profilerData, beginAt, endAt, options);
    this._populateCallTree(threadNode, options);
    this.emit(EVENTS.CALL_TREE_RENDERED);
  },

  


  _onRecordingStoppedOrSelected: function (_, recording) {
    
    if (!recording.isRecording()) {
      let profilerData = recording.getProfilerData();
      this.render(profilerData);
    }
  },

  


  _onRangeChange: function (_, params) {
    
    
    let recording = PerformanceController.getCurrentRecording();
    let profilerData = recording.getProfilerData();
    let { beginAt, endAt } = params || {};
    this.render(profilerData, beginAt, endAt);
  },

  


  _onLink: function (_, treeItem) {
    let { url, line } = treeItem.frame.getInfo();
    viewSourceInDebugger(url, line).then(
      () => this.emit(EVENTS.SOURCE_SHOWN_IN_JS_DEBUGGER),
      () => this.emit(EVENTS.SOURCE_NOT_FOUND_IN_JS_DEBUGGER));
  },

  



  _prepareCallTree: function (profilerData, beginAt, endAt, options) {
    let threadSamples = profilerData.profile.threads[0].samples;
    let contentOnly = !Prefs.showPlatformData;
    let invertTree = PerformanceController.getPref("invert-call-tree");

    let threadNode = new ThreadNode(threadSamples, contentOnly, beginAt, endAt, invertTree);
    options.inverted = invertTree && threadNode.samples > 0;

    return threadNode;
  },

  


  _populateCallTree: function (frameNode, options={}) {
    let root = new CallView({
      autoExpandDepth: options.inverted ? 0 : undefined,
      frame: frameNode,
      hidden: options.inverted,
      inverted: options.inverted
    });

    
    root.on("link", this._onLink);

    
    this._callTree.innerHTML = "";
    root.attachTo(this._callTree);

    let contentOnly = !Prefs.showPlatformData;
    root.toggleCategories(!contentOnly);
  },

  


  _onPrefChanged: function (_, prefName, value) {
    if (prefName === "invert-call-tree") {
      let { beginAt, endAt } = OverviewView.getRange();
      let profilerData = PerformanceController.getCurrentRecording().getProfilerData();
      this.render(profilerData, beginAt || void 0, endAt || void 0);
    }
  }
};




EventEmitter.decorate(CallTreeView);







let viewSourceInDebugger = Task.async(function *(url, line) {
  
  
  
  let debuggerAlreadyOpen = gToolbox.getPanel("jsdebugger");
  let { panelWin: dbg } = yield gToolbox.selectTool("jsdebugger");

  if (!debuggerAlreadyOpen) {
    yield dbg.once(dbg.EVENTS.SOURCES_ADDED);
  }

  let { DebuggerView } = dbg;
  let { Sources } = DebuggerView;

  let item = Sources.getItemForAttachment(a => a.source.url === url);
  if (item) {
    return DebuggerView.setEditorLocation(item.attachment.source.actor, line, { noDebug: true });
  }

  return Promise.reject("Couldn't find the specified source in the debugger.");
});
