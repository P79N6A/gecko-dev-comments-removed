








var loop = loop || {};
loop.webapp = (function($, _, OT, webL10n) {
  "use strict";

  loop.config = loop.config || {};
  loop.config.serverUrl = loop.config.serverUrl || "http://localhost:5000";

  var sharedModels = loop.shared.models,
      sharedViews = loop.shared.views,
      baseServerUrl = loop.config.serverUrl,
      __ = webL10n.get;

  



  var router;

  


  var HomeView = sharedViews.BaseView.extend({
    template: _.template('<p data-l10n-id="welcome"></p>')
  });

  


  var PromoteFirefoxView = React.createClass({displayName: 'PromoteFirefoxView',
    propTypes: {
      helper: React.PropTypes.object.isRequired
    },

    render: function() {
      if (this.props.helper.isFirefox(navigator.userAgent)) {
        return React.DOM.div(null );
      }
      return (
        React.DOM.div( {className:"promote-firefox"}, 
          React.DOM.h3(null, __("promote_firefox_hello_heading")),
          React.DOM.p(null, 
            React.DOM.a( {className:"btn btn-large btn-success",
               href:"https://www.mozilla.org/firefox/"}, 
              __("get_firefox_button")
            )
          )
        )
      );
    }
  });

  


  var CallUrlExpiredView = React.createClass({displayName: 'CallUrlExpiredView',
    propTypes: {
      helper: React.PropTypes.object.isRequired
    },

    render: function() {
      
      return (
        React.DOM.div( {className:"expired-url-info"}, 
          React.DOM.div( {className:"info-panel"}, 
            React.DOM.div( {className:"firefox-logo"} ),
            React.DOM.h1(null, __("call_url_unavailable_notification_heading")),
            React.DOM.h4(null, __("call_url_unavailable_notification_message"))
          ),
          PromoteFirefoxView( {helper:this.props.helper} )
        )
      );
      
    }
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
        client: new loop.StandaloneClient({
          baseServerUrl: baseServerUrl
        }),
        outgoing: true,
        
        
        callType: "audio-video"
      });
      this.disableForm();
    }
  });

  


  var WebappRouter = loop.shared.router.BaseConversationRouter.extend({
    routes: {
      "":                    "home",
      "unsupportedDevice":   "unsupportedDevice",
      "unsupportedBrowser":  "unsupportedBrowser",
      "call/expired":        "expired",
      "call/ongoing/:token": "loadConversation",
      "call/:token":         "initiate"
    },

    initialize: function(options) {
      this.helper = options.helper;
      if (!this.helper) {
        throw new Error("WebappRouter requires an helper object");
      }

      
      this.loadView(new HomeView());

      this.listenTo(this._conversation, "timeout", this._onTimeout);
      this.listenTo(this._conversation, "session:expired",
                    this._onSessionExpired);
    },

    _onSessionExpired: function() {
      this.navigate("/call/expired", {trigger: true});
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

    _onTimeout: function() {
      this._notifier.errorL10n("call_timeout_notification_text");
    },

    


    home: function() {
      this.loadView(new HomeView());
    },

    unsupportedDevice: function() {
      this.loadView(new sharedViews.UnsupportedDeviceView());
    },

    unsupportedBrowser: function() {
      this.loadView(new sharedViews.UnsupportedBrowserView());
    },

    expired: function() {
      this.loadReactComponent(CallUrlExpiredView({helper: this.helper}));
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
      this.loadReactComponent(sharedViews.ConversationView({
        sdk: OT,
        model: this._conversation
      }));
    }
  });

  


  function WebappHelper() {
    this._iOSRegex = /^(iPad|iPhone|iPod)/;
  }

  WebappHelper.prototype = {
    isFirefox: function(platform) {
      return platform.indexOf("Firefox") !== -1;
    },

    isIOS: function(platform) {
      return this._iOSRegex.test(platform);
    }
  };

  


  function init() {
    var helper = new WebappHelper();
    router = new WebappRouter({
      helper: helper,
      notifier: new sharedViews.NotificationListView({el: "#messages"}),
      conversation: new sharedModels.ConversationModel({}, {
        sdk: OT,
        pendingCallTimeout: loop.config.pendingCallTimeout
      })
    });
    Backbone.history.start();
    if (helper.isIOS(navigator.platform)) {
      router.navigate("unsupportedDevice", {trigger: true});
    } else if (!OT.checkSystemRequirements()) {
      router.navigate("unsupportedBrowser", {trigger: true});
    }
  }

  return {
    baseServerUrl: baseServerUrl,
    CallUrlExpiredView: CallUrlExpiredView,
    ConversationFormView: ConversationFormView,
    HomeView: HomeView,
    init: init,
    PromoteFirefoxView: PromoteFirefoxView,
    WebappHelper: WebappHelper,
    WebappRouter: WebappRouter
  };
})(jQuery, _, window.OT, document.webL10n);
