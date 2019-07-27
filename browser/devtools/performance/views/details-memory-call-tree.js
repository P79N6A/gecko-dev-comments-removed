


"use strict";




let MemoryCallTreeView = Heritage.extend(DetailsSubview, {

  rerenderPrefs: [
    "invert-call-tree"
  ],

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
    gToolbox.viewSourceInDebugger(url, line).then(success => {
      if (success) {
        this.emit(EVENTS.SOURCE_SHOWN_IN_JS_DEBUGGER);
      } else {
        this.emit(EVENTS.SOURCE_NOT_FOUND_IN_JS_DEBUGGER);
      }
    });
  },

  



  _prepareCallTree: function (allocations, { startTime, endTime }, options) {
    let samples = RecordingUtils.getSamplesFromAllocations(allocations);
    let invertTree = PerformanceController.getOption("invert-call-tree");

    let threadNode = new ThreadNode(samples,
      { startTime, endTime, invertTree });

    
    
    
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
      
      
      visibleCells: {
        allocations: true,
        selfAllocations: true,
        function: true
      }
    });

    
    root.on("link", this._onLink);

    
    root.on("focus", () => this.emit("focus"));

    
    let container = $("#memory-calltree-view > .call-tree-cells-container");
    container.innerHTML = "";
    root.attachTo(container);

    
    root.toggleCategories(false);
  },

  toString: () => "[object MemoryCallTreeView]"
});
