





var loop = loop || {};
loop.webapp = (function($, TB, webl10n) {
  "use strict";

  






  var sharedModels = loop.shared.models,
      sharedViews = loop.shared.views,
      
      
      baseServerUrl = "http://localhost:5000",
      
      __ = webl10n.get;

  



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

    








    initialize: function(options) {
      options = options || {};

      if (!options.model) {
        throw new Error("missing required model");
      }
      this.model = options.model;

      if (!options.notifier) {
        throw new Error("missing required notifier");
      }
      this.notifier = options.notifier;

      this.listenTo(this.model, "session:error", this._onSessionError);
    },

    _onSessionError: function(error) {
      console.error(error);
      this.notifier.notify({
        message: __("unable_retrieve_call_info"),
        level: "error"
      });
    },

    




    disableForm: function() {
      this.$("button").attr("disabled", "disabled");
    },

    initiate: function(event) {
      event.preventDefault();
      this.model.initiate({
        baseServerUrl: baseServerUrl,
        outgoing: true
      });
      this.disableForm();
    }
  });

  





  var WebappRouter = loop.shared.router.BaseRouter.extend({
    



    _conversation: undefined,

    



    _notifier: undefined,

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

      if (!options.notifier) {
        throw new Error("missing required notifier");
      }
      this._notifier = options.notifier;

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
      this.loadView(new ConversationFormView({
        model: this._conversation,
        notifier: this._notifier
      }));
    },

    



    conversation: function() {
      if (!this._conversation.isSessionReady()) {
        var loopToken = this._conversation.get("loopToken");
        if (loopToken) {
          return this.navigate("call/" + loopToken, {trigger: true});
        } else {
          this._notifier.notify({
            message: __("Missing conversation information"),
            level: "error"
          });
          return this.navigate("home", {trigger: true});
        }
      }
      this.loadView(new sharedViews.ConversationView({
        sdk: TB,
        model: this._conversation
      }));
    }
  });

  


  function init() {
    conversation = new sharedModels.ConversationModel();
    router = new WebappRouter({
      conversation: conversation,
      notifier: new sharedViews.NotificationListView({el: "#messages"})
    });
    Backbone.history.start();
  }

  return {
    baseServerUrl: baseServerUrl,
    ConversationFormView: ConversationFormView,
    HomeView: HomeView,
    init: init,
    WebappRouter: WebappRouter
  };
})(jQuery, window.TB, document.webL10n);
