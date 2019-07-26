





var loop = loop || {};
loop.webapp = (function($, _, OT) {
  "use strict";

  






  var sharedModels = loop.shared.models,
      sharedViews = loop.shared.views,
      
      
      baseServerUrl = "http://localhost:5000";

  



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
      this.notifier.errorL10n("unable_retrieve_call_info");
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
      "unsupported":         "unsupported",
      "call/ongoing/:token": "loadConversation",
      "call/:token":         "initiate"
    },

    initialize: function() {
      
      this.loadView(new HomeView());
    },

    


    startCall: function() {
      if (!this._conversation.get("loopToken")) {
        this._notifier.errorL10n("missing_conversation_info");
        this.navigate("home", {trigger: true});
      } else {
        this.navigate("call/ongoing/" + this._conversation.get("loopToken"), {
          trigger: true
        });
      }
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

    unsupported: function() {
      this.loadView(new sharedViews.UnsupportedView());
    },

    






    initiate: function(loopToken) {
      
      if (this._conversation.get("ongoing")) {
        this._conversation.endSession();
      }
      this._conversation.set("loopToken", loopToken);
      this.loadView(new ConversationFormView({
        model: this._conversation,
        notifier: this._notifier
      }));
    },

    



    loadConversation: function(loopToken) {
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
      conversation: new sharedModels.ConversationModel({}, {sdk: OT}),
      notifier: new sharedViews.NotificationListView({el: "#messages"})
    });
    Backbone.history.start();
    if (!OT.checkSystemRequirements())
      router.navigate("unsupported", {trigger: true});
  }

  return {
    baseServerUrl: baseServerUrl,
    ConversationFormView: ConversationFormView,
    HomeView: HomeView,
    init: init,
    WebappRouter: WebappRouter
  };
})(jQuery, _, window.OT);
