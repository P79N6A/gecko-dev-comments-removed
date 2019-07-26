





Components.utils.import("resource://gre/modules/Services.jsm");

var loop = loop || {};
loop.conversation = (function(TB, mozL10n) {
  "use strict";

  var sharedViews = loop.shared.views,
      baseServerUrl = Services.prefs.getCharPref("loop.server"),
      
      __ = mozL10n.get;

  



  var router;

  



  var conversation;

  






  var ConversationRouter = loop.shared.router.BaseRouter.extend({
    



    _conversation: undefined,

    



    _notifier: undefined,

    routes: {
      "start/:version": "start",
      "call/ongoing": "conversation",
      "call/ended": "ended"
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
      this.listenTo(this._conversation, "session:peer-hung", this._onPeerHung);
      this.listenTo(this._conversation, "session:network-disconnected",
                                        this._onNetworkDisconnected);
    },

    


    _onSessionReady: function() {
      this.navigate("call/ongoing", {trigger: true});
    },

    


    _onSessionEnded: function() {
      this.navigate("call/ended", {trigger: true});
    },

    








    _onPeerHung: function(event) {
      this._notifier.warn(__("peer_ended_conversation"));
      this.navigate("call/ended", {trigger: true});
    },

    



    _onNetworkDisconnected: function() {
      this._notifier.warn(__("network_disconnected"));
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
        this._notifier.error(__("cannot_start_call_session_not_ready"));
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
      notifier: new sharedViews.NotificationListView({el: "#messages"})
    });
    Backbone.history.start();
  }

  return {
    ConversationRouter: ConversationRouter,
    init: init
  };
})(window.TB, document.mozL10n);
