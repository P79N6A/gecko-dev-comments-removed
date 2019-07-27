





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.router = (function() {
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

    




    loadView: function(view) {
      this.clearActiveView();
      this._activeView = {type: "backbone", view: view.render().show()};
      this.updateView(this._activeView.view.$el);
    },

    




    loadReactComponent: function(reactComponent) {
      this.clearActiveView();
      this._activeView = {
        type: "react",
        view: React.renderComponent(reactComponent,
                                    document.querySelector("#main"))
      };
    },

    


    clearActiveView: function() {
      if (!this._activeView) {
        return;
      }
      if (this._activeView.type === "react") {
        React.unmountComponentAtNode(document.querySelector("#main"));
      } else {
        this._activeView.view.remove();
      }
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
      if (!options.client) {
        throw new Error("missing required client");
      }
      this._conversation = options.conversation;
      this._client = options.client;

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

    


    endCall: function() {},

    


    _onSessionEnded: function() {
      this.endCall();
    },

    







    _onPeerHungup: function() {
      this._notifier.warnL10n("peer_ended_conversation2");
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
})();
