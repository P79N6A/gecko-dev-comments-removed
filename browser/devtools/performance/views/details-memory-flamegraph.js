


"use strict";







let MemoryFlameGraphView = Heritage.extend(DetailsSubview, {
  


  initialize: Task.async(function* () {
    DetailsSubview.initialize.call(this);
  }),

  


  destroy: function () {
    DetailsSubview.destroy.call(this);
  },

  





  render: function (interval={}) {
    this.emit(EVENTS.MEMORY_FLAMEGRAPH_RENDERED);
  }
});
