


"use strict";





let DetailsView = {
  



  initialize: function () {
    this.views = {
      callTree: CallTreeView
    };

    
    return promise.all([
      CallTreeView.initialize()
    ]);
  },

  


  destroy: function () {
    return promise.all([
      CallTreeView.destroy()
    ]);
  }
};




EventEmitter.decorate(DetailsView);
