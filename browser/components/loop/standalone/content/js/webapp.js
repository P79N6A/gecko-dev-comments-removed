








var loop = loop || {};
loop.webapp = (function($, _, OT, mozL10n) {
  "use strict";

  loop.config = loop.config || {};
  loop.config.serverUrl = loop.config.serverUrl || "http://localhost:5000";

  var sharedModels = loop.shared.models,
      sharedViews = loop.shared.views,
      baseServerUrl = loop.config.serverUrl;

  



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
        return React.DOM.div(null);
      }
      return (
        React.DOM.div({className: "promote-firefox"}, 
          React.DOM.h3(null, mozL10n.get("promote_firefox_hello_heading")), 
          React.DOM.p(null, 
            React.DOM.a({className: "btn btn-large btn-accept", 
               href: "https://www.mozilla.org/firefox/"}, 
              mozL10n.get("get_firefox_button")
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
        React.DOM.div({className: "expired-url-info"}, 
          React.DOM.div({className: "info-panel"}, 
            React.DOM.div({className: "firefox-logo"}), 
            React.DOM.h1(null, mozL10n.get("call_url_unavailable_notification_heading")), 
            React.DOM.h4(null, mozL10n.get("call_url_unavailable_notification_message2"))
          ), 
          PromoteFirefoxView({helper: this.props.helper})
        )
      );
      
    }
  });

  var ConversationHeader = React.createClass({displayName: 'ConversationHeader',
    render: function() {
      var cx = React.addons.classSet;
      var conversationUrl = location.href;

      var urlCreationDateClasses = cx({
        "light-color-font": true,
        "call-url-date": true, 
        
        "hide": !this.props.urlCreationDateString.length
      });

      var callUrlCreationDateString = mozL10n.get("call_url_creation_date_label", {
        "call_url_creation_date": this.props.urlCreationDateString
      });

      return (
        
        React.DOM.header({className: "standalone-header container-box"}, 
          React.DOM.h1({className: "standalone-header-title"}, 
            React.DOM.strong(null, mozL10n.get("brandShortname")), " ", mozL10n.get("clientShortname")
          ), 
          React.DOM.div({className: "loop-logo", title: "Firefox WebRTC! logo"}), 
          React.DOM.h3({className: "call-url"}, 
            conversationUrl
          ), 
          React.DOM.h4({className: urlCreationDateClasses}, 
            callUrlCreationDateString
          )
        )
        
      );
    }
  });

  var ConversationFooter = React.createClass({displayName: 'ConversationFooter',
    render: function() {
      return (
        React.DOM.div({className: "standalone-footer container-box"}, 
          React.DOM.div({title: "Mozilla Logo", className: "footer-logo"})
        )
      );
    }
  });

  



  var StartConversationView = React.createClass({displayName: 'StartConversationView',
    








    getInitialProps: function() {
      return {showCallOptionsMenu: false};
    },

    getInitialState: function() {
      return {
        urlCreationDateString: '',
        disableCallButton: false,
        showCallOptionsMenu: this.props.showCallOptionsMenu
      };
    },

    propTypes: {
      model: React.PropTypes.instanceOf(sharedModels.ConversationModel)
                                       .isRequired,
      
      notifications: React.PropTypes.object.isRequired,
      client: React.PropTypes.object.isRequired
    },

    componentDidMount: function() {
      
      window.addEventListener("click", this.clickHandler);
      this.props.model.listenTo(this.props.model, "session:error",
                                this._onSessionError);
      this.props.client.requestCallUrlInfo(this.props.model.get("loopToken"),
                                           this._setConversationTimestamp);
    },

    _onSessionError: function(error) {
      console.error(error);
      this.props.notifications.errorL10n("unable_retrieve_call_info");
    },

    







    _initiateOutgoingCall: function(callType) {
      return function() {
        this.props.model.set("selectedCallType", callType);
        this.setState({disableCallButton: true});
        this.props.model.setupOutgoingCall();
      }.bind(this);
    },

    _setConversationTimestamp: function(err, callUrlInfo) {
      if (err) {
        this.props.notifications.errorL10n("unable_retrieve_call_info");
      } else {
        var date = (new Date(callUrlInfo.urlCreationDate * 1000));
        var options = {year: "numeric", month: "long", day: "numeric"};
        var timestamp = date.toLocaleDateString(navigator.language, options);
        this.setState({urlCreationDateString: timestamp});
      }
    },

    componentWillUnmount: function() {
      window.removeEventListener("click", this.clickHandler);
      localStorage.setItem("has-seen-tos", "true");
    },

    clickHandler: function(e) {
      if (!e.target.classList.contains('btn-chevron') &&
          this.state.showCallOptionsMenu) {
            this._toggleCallOptionsMenu();
      }
    },

    _toggleCallOptionsMenu: function() {
      var state = this.state.showCallOptionsMenu;
      this.setState({showCallOptionsMenu: !state});
    },

    render: function() {
      var tos_link_name = mozL10n.get("terms_of_use_link_text");
      var privacy_notice_name = mozL10n.get("privacy_notice_link_text");

      var tosHTML = mozL10n.get("legal_text_and_links", {
        "terms_of_use_url": "<a target=_blank href='" +
          "https://accounts.firefox.com/legal/terms'>" + tos_link_name + "</a>",
        "privacy_notice_url": "<a target=_blank href='" +
          "https://www.mozilla.org/privacy/'>" + privacy_notice_name + "</a>"
      });

      var dropdownMenuClasses = React.addons.classSet({
        "native-dropdown-large-parent": true,
        "standalone-dropdown-menu": true,
        "visually-hidden": !this.state.showCallOptionsMenu
      });
      var tosClasses = React.addons.classSet({
        "terms-service": true,
        hide: (localStorage.getItem("has-seen-tos") === "true")
      });

      return (
        
        React.DOM.div({className: "container"}, 
          React.DOM.div({className: "container-box"}, 

            ConversationHeader({
              urlCreationDateString: this.state.urlCreationDateString}), 

            React.DOM.p({className: "standalone-call-btn-label"}, 
              mozL10n.get("initiate_call_button_label2")
            ), 

            React.DOM.div({id: "messages"}), 

            React.DOM.div({className: "btn-group"}, 
              React.DOM.div({className: "flex-padding-1"}), 
              React.DOM.div({className: "standalone-btn-chevron-menu-group"}, 
                React.DOM.div({className: "btn-group-chevron"}, 
                  React.DOM.div({className: "btn-group"}, 

                    React.DOM.button({className: "btn btn-large btn-accept", 
                            onClick: this._initiateOutgoingCall("audio-video"), 
                            disabled: this.state.disableCallButton, 
                            title: mozL10n.get("initiate_audio_video_call_tooltip2")}, 
                      React.DOM.span({className: "standalone-call-btn-text"}, 
                        mozL10n.get("initiate_audio_video_call_button2")
                      ), 
                      React.DOM.span({className: "standalone-call-btn-video-icon"})
                    ), 

                    React.DOM.div({className: "btn-chevron", 
                         onClick: this._toggleCallOptionsMenu}
                    )

                  ), 

                  React.DOM.ul({className: dropdownMenuClasses}, 
                    React.DOM.li(null, 
                      


                      React.DOM.button({className: "start-audio-only-call", 
                              onClick: this._initiateOutgoingCall("audio"), 
                              disabled: this.state.disableCallButton}, 
                        mozL10n.get("initiate_audio_call_button2")
                      )
                    )
                  )

                )
              ), 
              React.DOM.div({className: "flex-padding-1"})
            ), 

            React.DOM.p({className: tosClasses, 
               dangerouslySetInnerHTML: {__html: tosHTML}})
          ), 

          ConversationFooter(null)
        )
        
      );
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
        throw new Error("WebappRouter requires a helper object");
      }

      
      this.loadView(new HomeView());

      this.listenTo(this._conversation, "timeout", this._onTimeout);
    },

    _onSessionExpired: function() {
      this.navigate("/call/expired", {trigger: true});
    },

    



    setupOutgoingCall: function() {
      var loopToken = this._conversation.get("loopToken");
      if (!loopToken) {
        this._notifications.errorL10n("missing_conversation_info");
        this.navigate("home", {trigger: true});
      } else {
        var callType = this._conversation.get("selectedCallType");

        this._conversation.once("call:outgoing", this.startCall, this);

        this._client.requestCallInfo(this._conversation.get("loopToken"),
                                     callType, function(err, sessionData) {
          if (err) {
            switch (err.errno) {
              
              
              
              case 105:
                this._onSessionExpired();
                break;
              default:
                this._notifications.errorL10n("missing_conversation_info");
                this.navigate("home", {trigger: true});
                break;
            }
            return;
          }
          this._conversation.outgoing(sessionData);
        }.bind(this));
      }
    },

    


    startCall: function() {
      var loopToken = this._conversation.get("loopToken");
      if (!loopToken) {
        this._notifications.errorL10n("missing_conversation_info");
        this.navigate("home", {trigger: true});
      } else {
        this._setupWebSocketAndCallView(loopToken);
      }
    },

    





    _setupWebSocketAndCallView: function(loopToken) {
      this._websocket = new loop.CallConnectionWebSocket({
        url: this._conversation.get("progressURL"),
        websocketToken: this._conversation.get("websocketToken"),
        callId: this._conversation.get("callId"),
      });
      this._websocket.promiseConnect().then(function() {
        this.navigate("call/ongoing/" + loopToken, {
          trigger: true
        });
      }.bind(this), function() {
        
        
        this._notifications.errorL10n("cannot_start_call_session_not_ready");
        return;
      }.bind(this));

      this._websocket.on("progress", this._handleWebSocketProgress, this);
    },

    



    _checkConnected: function() {
      
      
      if (this._conversation.streamsConnected()) {
        this._websocket.mediaUp();
      }
    },

    



    _handleWebSocketProgress: function(progressData) {
      if (progressData.state === "terminated") {
        
        
        
        
        
        
        
        
        switch (progressData.reason) {
          case "reject":
            this._handleCallRejected();
        }
      }
    },

    




    _handleCallRejected: function() {
      this.endCall();
      this._notifications.errorL10n("call_timeout_notification_text");
    },

    


    endCall: function() {
      var route = "home";
      if (this._conversation.get("loopToken")) {
        route = "call/" + this._conversation.get("loopToken");
      }
      this.navigate(route, {trigger: true});
    },

    _onTimeout: function() {
      this._notifications.errorL10n("call_timeout_notification_text");
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

      var startView = StartConversationView({
        model: this._conversation,
        notifications: this._notifications,
        client: this._client
      });
      this._conversation.once("call:outgoing:setup", this.setupOutgoingCall, this);
      this._conversation.once("change:publishedStream", this._checkConnected, this);
      this._conversation.once("change:subscribedStream", this._checkConnected, this);
      this.loadReactComponent(startView);
    },

    



    loadConversation: function(loopToken) {
      if (!this._conversation.isSessionReady()) {
        
        return this.navigate("call/" + loopToken, {trigger: true});
      }
      this.loadReactComponent(sharedViews.ConversationView({
        sdk: OT,
        model: this._conversation,
        video: {enabled: this._conversation.hasVideoStream("outgoing")}
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
    var client = new loop.StandaloneClient({
      baseServerUrl: baseServerUrl
    });
    var router = new WebappRouter({
      helper: helper,
      notifications: new sharedModels.NotificationCollection(),
      client: client,
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

    
    document.documentElement.lang = mozL10n.language.code;
    document.documentElement.dir = mozL10n.language.direction;
  }

  return {
    baseServerUrl: baseServerUrl,
    CallUrlExpiredView: CallUrlExpiredView,
    StartConversationView: StartConversationView,
    HomeView: HomeView,
    init: init,
    PromoteFirefoxView: PromoteFirefoxView,
    WebappHelper: WebappHelper,
    WebappRouter: WebappRouter
  };
})(jQuery, _, window.OT, navigator.mozL10n);
