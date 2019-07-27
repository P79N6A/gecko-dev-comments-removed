


"use strict";




let CallTreeView = {
  


  initialize: function () {
    this.el = $(".call-tree");
    this._graphEl = $(".call-tree-cells-container");
    this._stop = this._stop.bind(this);

    PerformanceController.on(EVENTS.RECORDING_STOPPED, this._stop);
  },

  


  destroy: function () {
    PerformanceController.off(EVENTS.RECORDING_STOPPED, this._stop);
  },

  _stop: function (_, { profilerData }) {
    this._prepareCallTree(profilerData);
  },

  



  _prepareCallTree: function (profilerData, beginAt, endAt, options={}) {
    let threadSamples = profilerData.profile.threads[0].samples;
    let contentOnly = !Prefs.showPlatformData;
    
    let invertTree = false;

    let threadNode = new ThreadNode(threadSamples, contentOnly, beginAt, endAt, invertTree);
    options.inverted = invertTree && threadNode.samples > 0;

    this._populateCallTree(threadNode, options);
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

    this.emit(EVENTS.CALL_TREE_RENDERED);
  }
};




EventEmitter.decorate(CallTreeView);
