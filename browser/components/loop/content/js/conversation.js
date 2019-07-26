





Components.utils.import("resource://gre/modules/Services.jsm");

var loop = loop || {};
loop.conversation = (function(OT, mozL10n) {
  "use strict";

  var sharedViews = loop.shared.views,
      baseServerUrl = Services.prefs.getCharPref("loop.server"),
      
      __ = mozL10n.get;

  



  var router;

  



  var EndedCallView = sharedViews.BaseView.extend({
    template: _.template([
      '<p>',
      '  <button class="btn btn-info" data-l10n-id="close_window"></button>',
      '</p>'
    ].join("")),

    className: "call-ended",

    events: {
      "click button": "closeWindow"
    },

    closeWindow: function(event) {
      event.preventDefault();
      
      window.close();
    }
  });

  








  var ConversationRouter = loop.shared.router.BaseConversationRouter.extend({
    routes: {
      "start/:version": "start",
      "call/ongoing": "conversation",
      "call/ended": "ended"
    },

    


    startCall: function() {
      this.navigate("call/ongoing", {trigger: true});
    },

    


    endCall: function() {
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
          sdk: OT,
          model: this._conversation
      }));
    },

    


    ended: function() {
      this.loadView(new EndedCallView());
    }
  });

  


  function init() {
    router = new ConversationRouter({
      conversation: new loop.shared.models.ConversationModel({}, {sdk: OT}),
      notifier: new sharedViews.NotificationListView({el: "#messages"})
    });
    Backbone.history.start();
  }

  return {
    ConversationRouter: ConversationRouter,
    EndedCallView: EndedCallView,
    init: init
  };
})(window.OT, document.mozL10n);
