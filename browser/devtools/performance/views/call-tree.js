


"use strict";




let CallTreeView = {
  


  initialize: function () {
    this.el = $(".call-tree");
    this._graphEl = $(".call-tree-cells-container");
    this._onRangeChange = this._onRangeChange.bind(this);
    this._stop = this._stop.bind(this);

    OverviewView.on(EVENTS.OVERVIEW_RANGE_SELECTED, this._onRangeChange);
    OverviewView.on(EVENTS.OVERVIEW_RANGE_CLEARED, this._onRangeChange);
    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._stop);
  },

  


  destroy: function () {
    OverviewView.off(EVENTS.OVERVIEW_RANGE_SELECTED, this._onRangeChange);
    OverviewView.off(EVENTS.OVERVIEW_RANGE_CLEARED, this._onRangeChange);
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._stop);
  },

  



  render: function (profilerData, beginAt, endAt, options={}) {
    let threadNode = this._prepareCallTree(profilerData, beginAt, endAt, options);
    this._populateCallTree(threadNode, options);
    this.emit(EVENTS.CALL_TREE_RENDERED);
  },

  


  _stop: function (_, { profilerData }) {
    this._profilerData = profilerData;
    this.render(profilerData);
  },

  


  _onRangeChange: function (_, params) {
    
    
    let { beginAt, endAt } = params || {};
    this.render(this._profilerData, beginAt, endAt);
  },

  



  _prepareCallTree: function (profilerData, beginAt, endAt, options) {
    let threadSamples = profilerData.profile.threads[0].samples;
    let contentOnly = !Prefs.showPlatformData;
    
    let invertTree = false;

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

    
    this._graphEl.innerHTML = "";
    root.attachTo(this._graphEl);

    let contentOnly = !Prefs.showPlatformData;
    root.toggleCategories(!contentOnly);
  }
};




EventEmitter.decorate(CallTreeView);
