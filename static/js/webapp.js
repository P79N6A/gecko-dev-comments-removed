





var loop = loop || {};
loop.webapp = (function() {
  "use strict";

  var router;

  


  var BaseView = Backbone.View.extend({
    hide: function() {
      this.$el.hide();
      return this;
    },

    show: function() {
      this.$el.show();
      return this;
    }
  });

  


  var HomeView = BaseView.extend({
    el: "#home"
  });

  


  var CallLauncherView = BaseView.extend({
    el: "#call-launcher",

    events: {
      "click button": "launchCall"
    },

    initialize: function(options) {
      options = options || {};
      if (!options.token) {
        throw new Error("missing required token");
      }
      this.token = options.token;
    },

    launchCall: function(event) {
      event.preventDefault();
      
    }
  });

  


  var CallView = BaseView.extend({
    el: "#call"
  });

  




  var Router = Backbone.Router.extend({
    view: undefined,

    routes: {
        "": "home",
        "call/:token": "call"
    },

    initialize: function() {
      this.loadView(new HomeView());
    },

    




    loadView : function(view) {
      this.view && this.view.hide();
      this.view = view.render().show();
    },

    


    home: function() {
      this.loadView(new HomeView());
    },

    




    call: function(token) {
      this.loadView(new CallLauncherView({token: token}));
    }
  });

  


  function init() {
    router = new Router();
    Backbone.history.start();
  }

  return {
    init: init,
    BaseView: BaseView,
    HomeView: HomeView,
    Router: Router,
    CallLauncherView: CallLauncherView
  };
})();
