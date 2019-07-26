





var loop = loop || {};
loop.webapp = (function($, TB) {
  "use strict";

  






  var sharedModels = loop.shared.models,
      sharedViews = loop.shared.views,
      
      
      baseServerUrl = "http://localhost:5000";

  



  var router;

  



  var conversation;

  


  var HomeView = sharedViews.BaseView.extend({
    el: "#home"
  });

  



  var ConversationFormView = sharedViews.BaseView.extend({
    el: "#conversation-form",

    events: {
      "submit": "initiate"
    },

    initialize: function() {
      this.listenTo(this.model, "session:error", function(error) {
        
        
        alert(error);
      });
    },

    initiate: function(event) {
      event.preventDefault();
      this.model.initiate({
        baseServerUrl: baseServerUrl,
        outgoing: true
      });
    }
  });

  





  var WebappRouter = loop.shared.router.BaseRouter.extend({
    _conversation: undefined,

    routes: {
      "": "home",
      "call/ongoing": "conversation",
      "call/:token": "initiate"
    },

    initialize: function(options) {
      options = options || {};
      if (!options.conversation) {
        throw new Error("missing required conversation");
      }
      this._conversation = options.conversation;

      this.listenTo(this._conversation, "session:ready", this._onSessionReady);
      this.listenTo(this._conversation, "session:ended", this._onSessionEnded);

      
      this.loadView(new HomeView());
    },

    


    _onSessionReady: function() {
      this.navigate("call/ongoing", {trigger: true});
    },

    


    _onSessionEnded: function() {
      this.navigate("call/" + this._conversation.get("token"), {trigger: true});
    },

    


    home: function() {
      this.loadView(new HomeView());
    },

    





    initiate: function(loopToken) {
      this._conversation.set("loopToken", loopToken);
      this.loadView(new ConversationFormView({model: this._conversation}));
    },

    



    conversation: function() {
      if (!this._conversation.isSessionReady()) {
        var loopToken = this._conversation.get("loopToken");
        if (loopToken) {
          return this.navigate("call/" + loopToken, {trigger: true});
        } else {
          
          return this.navigate("home", {trigger: true});
        }
      }
      this.loadView(
        new sharedViews.ConversationView({
          sdk: TB,
          model: this._conversation
        }));
    }
  });

  


  function init() {
    conversation = new sharedModels.ConversationModel();
    router = new WebappRouter({conversation: conversation});
    Backbone.history.start();
  }

  return {
    ConversationFormView: ConversationFormView,
    HomeView: HomeView,
    init: init,
    WebappRouter: WebappRouter
  };
})(jQuery, window.TB);
