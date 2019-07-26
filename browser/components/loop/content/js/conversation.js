





Components.utils.import("resource://gre/modules/Services.jsm");

var loop = loop || {};
loop.conversation = (function(TB) {
  "use strict";

  var baseServerUrl = Services.prefs.getCharPref("loop.server");

  



  var router;

  



  var conversation;

  var ConversationRouter = loop.shared.router.BaseRouter.extend({
    _conversation: undefined,
    activeView: undefined,

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
    router = new ConversationRouter({conversation: conversation});
    Backbone.history.start();
  }

  return {
    ConversationRouter: ConversationRouter,
    init: init
  };
})(window.TB);
