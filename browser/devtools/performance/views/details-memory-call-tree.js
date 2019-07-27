


"use strict";




let MemoryCallTreeView = Heritage.extend(DetailsSubview, {

  rerenderPrefs: [
    "invert-call-tree"
  ],

  rangeChangeDebounceTime: 100, 

  


  initialize: function () {
    DetailsSubview.initialize.call(this);

    this._onLink = this._onLink.bind(this);

    this.container = $("#memory-calltree-view > .call-tree-cells-container");
  },

  


  destroy: function () {
    DetailsSubview.destroy.call(this);
  },

  





  render: function (interval={}) {
    let options = {
      invertTree: PerformanceController.getOption("invert-call-tree")
    };
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
    let thread = RecordingUtils.getProfileThreadFromAllocations(allocations);
    let { invertTree } = options;

    return new ThreadNode(thread, { startTime, endTime, invertTree });
  },

  


  _populateCallTree: function (frameNode, options={}) {
    
    
    
    let inverted = options.invertTree && frameNode.samples > 0;

    let root = new CallView({
      frame: frameNode,
      inverted: inverted,
      
      hidden: inverted,
      
      sortingPredicate: (a, b) => a.frame.allocations < b.frame.allocations ? 1 : -1,
      
      
      autoExpandDepth: inverted ? 0 : undefined,
      
      
      visibleCells: {
        allocations: true,
        selfAllocations: true,
        function: true
      }
    });

    
    root.on("link", this._onLink);

    
    root.on("focus", () => this.emit("focus"));

    
    this.container.innerHTML = "";
    root.attachTo(this.container);

    
    root.toggleCategories(false);
  },

  toString: () => "[object MemoryCallTreeView]"
});

EventEmitter.decorate(MemoryCallTreeView);
