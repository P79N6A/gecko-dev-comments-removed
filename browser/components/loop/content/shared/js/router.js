





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.router = (function() {
  "use strict";

  





  var BaseRouter = Backbone.Router.extend({
    activeView: undefined,

    




    loadView : function(view) {
      if (this.activeView) {
        this.activeView.hide();
      }
      this.activeView = view.render().show();
    }
  });

  return {
    BaseRouter: BaseRouter
  };
})();
