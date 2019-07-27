


"use strict";




let JsCallTreeView = Heritage.extend(DetailsSubview, {

  rerenderPrefs: [
    "invert-call-tree",
    "show-platform-data"
  ],

  rangeChangeDebounceTime: 50, 

  


  initialize: function () {
    DetailsSubview.initialize.call(this);

    this._onPrefChanged = this._onPrefChanged.bind(this);
    this._onLink = this._onLink.bind(this);
  },

  


  destroy: function () {
    DetailsSubview.destroy.call(this);
  },

  







  render: function (interval={}, options={}) {
    let recording = PerformanceController.getCurrentRecording();
    let profile = recording.getProfile();
    let threadNode = this._prepareCallTree(profile, interval, options);
    this._populateCallTree(threadNode, options);
    this.emit(EVENTS.JS_CALL_TREE_RENDERED);
  },

  


  _onLink: function (_, treeItem) {
    let { url, line } = treeItem.frame.getInfo();
    viewSourceInDebugger(url, line).then(
      () => this.emit(EVENTS.SOURCE_SHOWN_IN_JS_DEBUGGER),
      () => this.emit(EVENTS.SOURCE_NOT_FOUND_IN_JS_DEBUGGER));
  },

  



  _prepareCallTree: function (profile, { startTime, endTime }, options) {
    let threadSamples = profile.threads[0].samples;
    let contentOnly = !PerformanceController.getOption("show-platform-data");
    let invertTree = PerformanceController.getOption("invert-call-tree");

    let threadNode = new ThreadNode(threadSamples,
      { startTime, endTime, contentOnly, invertTree });

    
    
    
    options.inverted = invertTree && threadNode.samples > 0;

    return threadNode;
  },

  


  _populateCallTree: function (frameNode, options={}) {
    let root = new CallView({
      frame: frameNode,
      inverted: options.inverted,
      
      hidden: options.inverted,
      
      
      autoExpandDepth: options.inverted ? 0 : undefined,
    });

    
    root.on("link", this._onLink);

    
    root.on("focus", () => this.emit("focus"));

    
    let container = $("#js-calltree-view > .call-tree-cells-container");
    container.innerHTML = "";
    root.attachTo(container);

    
    root.toggleAllocations(false);

    
    
    let contentOnly = !PerformanceController.getOption("show-platform-data");
    root.toggleCategories(!contentOnly);
  },

  toString: () => "[object JsCallTreeView]"
});







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
