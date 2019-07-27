


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

    this.container = $("#js-calltree-view .call-tree-cells-container");

    JITOptimizationsView.initialize();
  },

  


  destroy: function () {
    this.container = null;
    JITOptimizationsView.destroy();
    DetailsSubview.destroy.call(this);
  },

  





  render: function (interval={}) {
    let options = {
      contentOnly: !PerformanceController.getOption("show-platform-data"),
      invertTree: PerformanceController.getOption("invert-call-tree")
    };
    let recording = PerformanceController.getCurrentRecording();
    let profile = recording.getProfile();
    let threadNode = this._prepareCallTree(profile, interval, options);
    this._populateCallTree(threadNode, options);
    this.emit(EVENTS.JS_CALL_TREE_RENDERED);
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

  



  _prepareCallTree: function (profile, { startTime, endTime }, options) {
    let threadSamples = profile.threads[0].samples;
    let optimizations = profile.threads[0].optimizations;
    let { contentOnly, invertTree } = options;

    let threadNode = new ThreadNode(threadSamples,
      { startTime, endTime, contentOnly, invertTree, optimizations });

    return threadNode;
  },

  


  _populateCallTree: function (frameNode, options={}) {
    
    
    
    let inverted = options.invertTree && frameNode.samples > 0;

    let root = new CallView({
      frame: frameNode,
      inverted: inverted,
      
      hidden: inverted,
      
      
      autoExpandDepth: inverted ? 0 : undefined
    });

    
    root.on("link", this._onLink);

    
    
    root.on("focus", (_, node) => this.emit("focus", node));

    
    this.container.innerHTML = "";
    root.attachTo(this.container);

    
    
    root.toggleCategories(options.contentOnly);

    
    return root;
  },

  toString: () => "[object JsCallTreeView]"
});
