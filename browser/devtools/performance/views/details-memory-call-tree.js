


"use strict";




let MemoryCallTreeView = Heritage.extend(DetailsSubview, {

  rerenderPrefs: ["invert-call-tree", "show-platform-data"],

  rangeChangeDebounceTime: 100, 

  


  initialize: function () {
    DetailsSubview.initialize.call(this);

    this._onLink = this._onLink.bind(this);
  },

  


  destroy: function () {
    DetailsSubview.destroy.call(this);
  },

  







  render: function (interval={}, options={}) {
    let recording = PerformanceController.getCurrentRecording();
    let allocations = recording.getAllocations();
    let threadNode = this._prepareCallTree(allocations, interval, options);
    this._populateCallTree(threadNode, options);
    this.emit(EVENTS.MEMORY_CALL_TREE_RENDERED);
  },

  


  _onLink: function (_, treeItem) {
    let { url, line } = treeItem.frame.getInfo();
    viewSourceInDebugger(url, line).then(
      () => this.emit(EVENTS.SOURCE_SHOWN_IN_JS_DEBUGGER),
      () => this.emit(EVENTS.SOURCE_NOT_FOUND_IN_JS_DEBUGGER));
  },

  



  _prepareCallTree: function (allocations, { startTime, endTime }, options) {
    let samples = RecordingUtils.getSamplesFromAllocations(allocations);
    let contentOnly = !PerformanceController.getPref("show-platform-data");
    let invertTree = PerformanceController.getPref("invert-call-tree");

    let threadNode = new ThreadNode(samples,
      { startTime, endTime, contentOnly, invertTree });

    
    
    
    options.inverted = invertTree && threadNode.samples > 0;

    return threadNode;
  },

  


  _populateCallTree: function (frameNode, options={}) {
    let root = new CallView({
      frame: frameNode,
      inverted: options.inverted,
      
      hidden: options.inverted,
      
      sortingPredicate: (a, b) => a.frame.allocations < b.frame.allocations ? 1 : -1,
      
      
      autoExpandDepth: options.inverted ? 0 : undefined,
    });

    
    root.on("link", this._onLink);

    
    let container = $("#memory-calltree-view > .call-tree-cells-container");
    container.innerHTML = "";
    root.attachTo(container);

    
    root.toggleCategories(false);
  }
});
