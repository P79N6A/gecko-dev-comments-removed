


"use strict";

let OptimizationsView = Heritage.extend(DetailsSubview, {

  rerenderPrefs: [
    "show-platform-data",
    "flatten-tree-recursion",
  ],

  rangeChangeDebounceTime: 75, 

  


  initialize: function () {
    DetailsSubview.initialize.call(this);
    this.reset = this.reset.bind(this);
    this.tabs = $("#optimizations-tabs");
    this._onFramesListSelect = this._onFramesListSelect.bind(this);

    OptimizationsListView.initialize();
    FramesListView.initialize({ container: $("#frames-tabpanel") });
    FramesListView.on("select", this._onFramesListSelect);
  },

  


  destroy: function () {
    DetailsSubview.destroy.call(this);
    this.tabs = this._threadNode = this._frameNode = null;

    FramesListView.off("select", this._onFramesListSelect);
    FramesListView.destroy();
    OptimizationsListView.destroy();
  },

  





  selectTabByName: function (name="frames") {
    switch(name) {
    case "optimizations":
      this.tabs.selectedIndex = 0;
      break;
    case "frames":
      this.tabs.selectedIndex = 1;
      break;
    }
  },

  





  render: function (interval={}) {
    let options = {
      contentOnly: !PerformanceController.getOption("show-platform-data"),
      flattenRecursion: PerformanceController.getOption("flatten-tree-recursion"),
      
      
      invertTree: true,
    };
    let recording = PerformanceController.getCurrentRecording();
    let profile = recording.getProfile();

    this.reset();
    
    
    this.threadNode = this._prepareThreadNode(profile, interval, options);
    this.emit(EVENTS.OPTIMIZATIONS_RENDERED);
  },

  



  set threadNode(threadNode) {
    if (threadNode === this._threadNode) {
      return;
    }
    this._threadNode = threadNode;
    
    
    this.frameNode = null;
    this._setAndRenderFramesList();
  },
  get threadNode() {
    return this._threadNode;
  },

  



  set frameNode(frameNode) {
    if (frameNode === this._frameNode) {
      return;
    }
    this._frameNode = frameNode;

    
    
    
    this.selectTabByName(frameNode ? "optimizations" : "frames");
    this._setAndRenderTierGraph();
    this._setAndRenderOptimizationsList();
  },

  get frameNode() {
    return this._frameNode;
  },

  



  reset: function () {
    this.threadNode = this.frameNode = null;
  },

  



  _prepareThreadNode: function (profile, { startTime, endTime }, options) {
    let thread = profile.threads[0];
    let { contentOnly, invertTree, flattenRecursion } = options;
    let threadNode = new ThreadNode(thread, { startTime, endTime, contentOnly, invertTree, flattenRecursion });
    return threadNode;
  },

  


  _setAndRenderTierGraph: function () {
    
  },

  


  _setAndRenderFramesList: function () {
    FramesListView.setCurrentThread(this.threadNode);
    FramesListView.render();
  },

  


  _setAndRenderOptimizationsList: function () {
    OptimizationsListView.setCurrentFrame(this.frameNode);
    OptimizationsListView.render();
  },

  


  _onFramesListSelect: function (_, frameNode) {
    this.frameNode = frameNode;
  },

  toString: () => "[object OptimizationsView]"
});

EventEmitter.decorate(OptimizationsView);
