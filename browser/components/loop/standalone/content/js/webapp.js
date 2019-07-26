





var loop = loop || {};
loop.webapp = (function($, OT, webl10n) {
  "use strict";

  






  var sharedModels = loop.shared.models,
      sharedViews = loop.shared.views,
      
      
      baseServerUrl = "http://localhost:5000",
      
      __ = webl10n.get;

  



  var router;

  


  var HomeView = sharedViews.BaseView.extend({
    template: _.template('<p data-l10n-id="welcome"></p>')
  });

  



  var ConversationFormView = sharedViews.BaseView.extend({
    template: _.template([
      '<form>',
      '  <p>',
      '    <button class="btn btn-success" data-l10n-id="start_call"></button>',
      '  </p>',
      '</form>'
    ].join("")),

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
      this.notifier.error(__("unable_retrieve_call_info"));
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

  


  var WebappRouter = loop.shared.router.BaseConversationRouter.extend({
    routes: {
      "":                    "home",
      "call/ongoing/:token": "conversation",
      "call/:token":         "initiate"
    },

    initialize: function() {
      
      this.loadView(new HomeView());
    },

    


    startCall: function() {
      var route;
      if (this._conversation.get("loopToken")) {
        route = "call/ongoing/" + this._conversation.get("loopToken");
      } else {
        route = "home";
        this._notifier.error(__("missing_conversation_info"));
      }
      this.navigate(route, {trigger: true});
    },

    


    endCall: function() {
      var route = "home";
      if (this._conversation.get("loopToken")) {
        route = "call/" + this._conversation.get("loopToken");
      }
      this.navigate(route, {trigger: true});
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

    



    conversation: function(loopToken) {
      if (!this._conversation.isSessionReady()) {
        
        return this.navigate("call/" + loopToken, {trigger: true});
      }
      this.loadView(new sharedViews.ConversationView({
        sdk: OT,
        model: this._conversation
      }));
    }
  });

  


  function init() {
    router = new WebappRouter({
      conversation: new sharedModels.ConversationModel(),
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
})(jQuery, window.OT, document.webL10n);
