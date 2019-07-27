






var loop = loop || {};
loop.conversation = (function(OT, mozL10n) {
  "use strict";

  var sharedViews = loop.shared.views;

  



  var router;

  



  var IncomingCallView = sharedViews.BaseView.extend({
    template: _.template([
      '<h2 data-l10n-id="incoming_call"></h2>',
      '<p>',
      '  <button class="btn btn-success btn-accept"',
      '           data-l10n-id="accept_button"></button>',
      '  <button class="btn btn-error btn-decline"',
      '           data-l10n-id="decline_button"></button>',
      '</p>'
    ].join("")),

    className: "incoming-call",

    events: {
      "click .btn-accept": "handleAccept",
      "click .btn-decline": "handleDecline"
    },

    



    handleAccept: function(event) {
      event.preventDefault();
      this.model.trigger("accept");
    },

    



    handleDecline: function(event) {
      event.preventDefault();
      this.model.trigger("decline");
    }
  });

  



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

  








  var ConversationRouter = loop.desktopRouter.DesktopConversationRouter.extend({
    routes: {
      "incoming/:version": "incoming",
      "call/accept": "accept",
      "call/decline": "decline",
      "call/ongoing": "conversation",
      "call/ended": "ended"
    },

    


    startCall: function() {
      this.navigate("call/ongoing", {trigger: true});
    },

    


    endCall: function() {
      this.navigate("call/ended", {trigger: true});
    },

    





    incoming: function(loopVersion) {
      window.navigator.mozLoop.startAlerting();
      this._conversation.set({loopVersion: loopVersion});
      this._conversation.once("accept", function() {
        this.navigate("call/accept", {trigger: true});
      }.bind(this));
      this._conversation.once("decline", function() {
        this.navigate("call/decline", {trigger: true});
      }.bind(this));
      this.loadView(new IncomingCallView({model: this._conversation}));
    },

    


    accept: function() {
      window.navigator.mozLoop.stopAlerting();
      this._conversation.initiate({
        client: new loop.Client(),
        outgoing: false
      });
    },

    


    decline: function() {
      window.navigator.mozLoop.stopAlerting();
      
      window.close();
    },

    



    conversation: function() {
      if (!this._conversation.isSessionReady()) {
        console.error("Error: navigated to conversation route without " +
          "the start route to initialise the call first");
        this._notifier.errorL10n("cannot_start_call_session_not_ready");
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
    
    
    mozL10n.initialize(window.navigator.mozLoop);

    document.title = mozL10n.get("incoming_call_title");

    router = new ConversationRouter({
      conversation: new loop.shared.models.ConversationModel({}, {sdk: OT}),
      notifier: new sharedViews.NotificationListView({el: "#messages"})
    });
    Backbone.history.start();
  }

  return {
    ConversationRouter: ConversationRouter,
    EndedCallView: EndedCallView,
    IncomingCallView: IncomingCallView,
    init: init
  };
})(window.OT, document.mozL10n);
