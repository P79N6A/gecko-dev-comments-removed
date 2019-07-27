


"use strict";




let MemoryCallTreeView = Heritage.extend(DetailsSubview, {
  rangeChangeDebounceTime: 100, 

  


  initialize: function () {
    DetailsSubview.initialize.call(this);

    this._cache = new WeakMap();

    this._onPrefChanged = this._onPrefChanged.bind(this);
    this._onLink = this._onLink.bind(this);

    PerformanceController.on(EVENTS.PREF_CHANGED, this._onPrefChanged);
  },

  


  destroy: function () {
    DetailsSubview.destroy.call(this);

    PerformanceController.off(EVENTS.PREF_CHANGED, this._onPrefChanged);
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
    let cached = this._cache.get(allocations);
    if (cached) {
      var samples = cached;
    } else {
      var samples = RecordingUtils.getSamplesFromAllocations(allocations);
      this._cache.set(allocations, samples);
    }

    let contentOnly = !Prefs.showPlatformData;
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
  },

  


  _onPrefChanged: function (_, prefName, value) {
    if (prefName === "invert-call-tree") {
      this.render(OverviewView.getTimeInterval());
    }
  }
});
