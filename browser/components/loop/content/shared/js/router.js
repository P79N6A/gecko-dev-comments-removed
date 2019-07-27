





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.router = (function(l10n) {
  "use strict";

  





  var BaseRouter = Backbone.Router.extend({
    



    _activeView: undefined,

    



    _notifier: undefined,

    







    constructor: function(options) {
      options = options || {};
      if (!options.notifier) {
        throw new Error("missing required notifier");
      }
      this._notifier = options.notifier;

      Backbone.Router.apply(this, arguments);
    },

    




    loadView : function(view) {
      if (this._activeView) {
        this._activeView.remove();
      }
      this._activeView = view.render().show();
      this.updateView(this._activeView.$el);
    },

    




    updateView: function($el) {
      $("#main").html($el);
    }
  });

  



  var BaseConversationRouter = BaseRouter.extend({
    



    _conversation: undefined,

    









    constructor: function(options) {
      options = options || {};
      if (!options.conversation) {
        throw new Error("missing required conversation");
      }
      this._conversation = options.conversation;

      this.listenTo(this._conversation, "session:ready", this._onSessionReady);
      this.listenTo(this._conversation, "session:ended", this._onSessionEnded);
      this.listenTo(this._conversation, "session:peer-hungup",
                                        this._onPeerHungup);
      this.listenTo(this._conversation, "session:network-disconnected",
                                        this._onNetworkDisconnected);
      this.listenTo(this._conversation, "session:connection-error",
                    this._notifyError);

      BaseRouter.apply(this, arguments);
    },

    



    _notifyError: function(error) {
      console.log(error);
      this._notifier.errorL10n("connection_error_see_console_notification");
      this.endCall();
    },

    


    startCall: function() {},

    


    endCall: function() {},

    


    _onSessionReady: function() {
      this.startCall();
    },

    


    _onSessionEnded: function() {
      this._notifier.warnL10n("call_has_ended");
      this.endCall();
    },

    







    _onPeerHungup: function() {
      this._notifier.warnL10n("peer_ended_conversation");
      this.endCall();
    },

    


    _onNetworkDisconnected: function() {
      this._notifier.warnL10n("network_disconnected");
      this.endCall();
    }
  });

  return {
    BaseRouter: BaseRouter,
    BaseConversationRouter: BaseConversationRouter
  };
})(document.webL10n || document.mozL10n);
