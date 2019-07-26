





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.router = (function(l10n) {
  "use strict";

  var __ = l10n.get;

  





  var BaseRouter = Backbone.Router.extend({
    activeView: undefined,

    




    loadView : function(view) {
      if (this.activeView) {
        this.activeView.hide();
      }
      this.activeView = view.render().show();
    }
  });

  



  var BaseConversationRouter = BaseRouter.extend({
    



    _conversation: undefined,

    



    _notifier: undefined,

    










    constructor: function(options) {
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
      this.listenTo(this._conversation, "session:peer-hungup",
                                        this._onPeerHungup);
      this.listenTo(this._conversation, "session:network-disconnected",
                                        this._onNetworkDisconnected);

      BaseRouter.apply(this, arguments);
    },

    


    startCall: function() {},

    


    endCall: function() {},

    


    _onSessionReady: function() {
      this.startCall();
    },

    


    _onSessionEnded: function() {
      this._notifier.warn(__("call_has_ended"));
      this.endCall();
    },

    







    _onPeerHungup: function() {
      this._notifier.warn(__("peer_ended_conversation"));
      this.endCall();
    },

    


    _onNetworkDisconnected: function() {
      this._notifier.warn(__("network_disconnected"));
      this.endCall();
    }
  });

  return {
    BaseRouter: BaseRouter,
    BaseConversationRouter: BaseConversationRouter
  };
})(document.webL10n || document.mozL10n);
