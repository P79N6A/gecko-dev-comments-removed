





Components.utils.import("resource://gre/modules/Services.jsm");

var loop = loop || {};
loop.conversation = (function(TB, mozl10n) {
  "use strict";

  var baseServerUrl = Services.prefs.getCharPref("loop.server"),
      
      __ = mozl10n.get;

  



  var router;

  



  var conversation;

  






  var ConversationRouter = loop.shared.router.BaseRouter.extend({
    _conversation: undefined,
    _notifier:     undefined,
    activeView:    undefined,

    routes: {
      "start/:version": "start",
      "call/ongoing": "conversation",
      "call/ended": "ended"
    },

    




    loadView : function(view) {
      if (this.activeView) {
        this.activeView.hide();
      }
      this.activeView = view.render().show();
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
    },

    


    _onSessionReady: function() {
      this.navigate("call/ongoing", {trigger: true});
    },

    


    _onSessionEnded: function() {
      this.navigate("call/ended", {trigger: true});
    },

    






    start: function(loopVersion) {
      
      
      this._conversation.set({loopVersion: loopVersion});
      this._conversation.initiate({
        baseServerUrl: baseServerUrl,
        outgoing: false
      });
    },

    



    conversation: function() {
      if (!this._conversation.isSessionReady()) {
        
        console.error("Error: navigated to conversation route without " +
          "the start route to initialise the call first");
        this._notifier.notify(__("cannot_start_call_session_not_ready"));
        return;
      }

      this.loadView(
        new loop.shared.views.ConversationView({
          sdk: TB,
          model: this._conversation
      }));
    },

    


    ended: function() {
      
      window.close();
    }
  });

  


  function init() {
    conversation = new loop.shared.models.ConversationModel();
    router = new ConversationRouter({
      conversation: conversation,
      notifier: new loop.shared.views.NotificationListView({
        el: "#messages"
      })
    });
    Backbone.history.start();
  }

  return {
    ConversationRouter: ConversationRouter,
    init: init
  };
})(window.TB, document.mozL10n);
