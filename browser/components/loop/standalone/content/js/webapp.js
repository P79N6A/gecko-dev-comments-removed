








var loop = loop || {};
loop.webapp = (function($, _, OT, mozL10n) {
  "use strict";

  loop.config = loop.config || {};
  loop.config.serverUrl = loop.config.serverUrl || "http://localhost:5000";

  var sharedModels = loop.shared.models,
      sharedViews = loop.shared.views;

  



  var router;

  


  var HomeView = React.createClass({displayName: 'HomeView',
    render: function() {
      return (
        React.DOM.p(null, mozL10n.get("welcome"))
      )
    }
  });

  


  var UnsupportedBrowserView = React.createClass({displayName: 'UnsupportedBrowserView',
    render: function() {
      var useLatestFF = mozL10n.get("use_latest_firefox", {
        "firefoxBrandNameLink": React.renderComponentToStaticMarkup(
          React.DOM.a({target: "_blank", href: "https://www.mozilla.org/firefox/"}, "Firefox")
        )
      });
      return (
        React.DOM.div(null, 
          React.DOM.h2(null, mozL10n.get("incompatible_browser")), 
          React.DOM.p(null, mozL10n.get("powered_by_webrtc")), 
          React.DOM.p({dangerouslySetInnerHTML: {__html: useLatestFF}})
        )
      );
    }
  });

  


  var UnsupportedDeviceView = React.createClass({displayName: 'UnsupportedDeviceView',
    render: function() {
      return (
        React.DOM.div(null, 
          React.DOM.h2(null, mozL10n.get("incompatible_device")), 
          React.DOM.p(null, mozL10n.get("sorry_device_unsupported")), 
          React.DOM.p(null, mozL10n.get("use_firefox_windows_mac_linux"))
        )
      );
    }
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

  var ConversationBranding = React.createClass({displayName: 'ConversationBranding',
    render: function() {
      return (
        React.DOM.h1({className: "standalone-header-title"}, 
          React.DOM.strong(null, mozL10n.get("brandShortname")), " ", mozL10n.get("clientShortname")
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
        
        React.DOM.header({className: "standalone-header header-box container-box"}, 
          ConversationBranding(null), 
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

  var PendingConversationView = React.createClass({displayName: 'PendingConversationView',
    getInitialState: function() {
      return {
        callState: this.props.callState || "connecting"
      }
    },

    propTypes: {
      websocket: React.PropTypes.instanceOf(loop.CallConnectionWebSocket)
                      .isRequired
    },

    componentDidMount: function() {
      this.props.websocket.listenTo(this.props.websocket, "progress:alerting",
                                    this._handleRingingProgress);
    },

    _handleRingingProgress: function() {
      this.setState({callState: "ringing"});
    },

    _cancelOutgoingCall: function() {
      this.props.websocket.cancel();
    },

    render: function() {
      var callState = mozL10n.get("call_progress_" + this.state.callState + "_description");
      return (
        
        React.DOM.div({className: "container"}, 
          React.DOM.div({className: "container-box"}, 
            React.DOM.header({className: "pending-header header-box"}, 
              ConversationBranding(null)
            ), 

            React.DOM.div({id: "cameraPreview"}), 

            React.DOM.div({id: "messages"}), 

            React.DOM.p({className: "standalone-btn-label"}, 
              callState
            ), 

            React.DOM.div({className: "btn-pending-cancel-group btn-group"}, 
              React.DOM.div({className: "flex-padding-1"}), 
              React.DOM.button({className: "btn btn-large btn-cancel", 
                      onClick: this._cancelOutgoingCall}, 
                React.DOM.span({className: "standalone-call-btn-text"}, 
                  mozL10n.get("initiate_call_cancel_button")
                )
              ), 
              React.DOM.div({className: "flex-padding-1"})
            )
          ), 

          ConversationFooter(null)
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

            React.DOM.p({className: "standalone-btn-label"}, 
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

  





  var OutgoingConversationView = React.createClass({displayName: 'OutgoingConversationView',
    propTypes: {
      client: React.PropTypes.instanceOf(loop.StandaloneClient).isRequired,
      conversation: React.PropTypes.instanceOf(sharedModels.ConversationModel)
                         .isRequired,
      helper: React.PropTypes.instanceOf(WebappHelper).isRequired,
      notifications: React.PropTypes.instanceOf(sharedModels.NotificationCollection)
                          .isRequired,
      sdk: React.PropTypes.object.isRequired
    },

    getInitialState: function() {
      return {
        callStatus: "start"
      };
    },

    componentDidMount: function() {
      this.props.conversation.on("call:outgoing", this.startCall, this);
      this.props.conversation.on("call:outgoing:setup", this.setupOutgoingCall, this);
      this.props.conversation.on("change:publishedStream", this._checkConnected, this);
      this.props.conversation.on("change:subscribedStream", this._checkConnected, this);
      this.props.conversation.on("session:ended", this._endCall, this);
      this.props.conversation.on("session:peer-hungup", this._onPeerHungup, this);
      this.props.conversation.on("session:network-disconnected", this._onNetworkDisconnected, this);
      this.props.conversation.on("session:connection-error", this._notifyError, this);
    },

    componentDidUnmount: function() {
      this.props.conversation.off(null, null, this);
    },

    


    render: function() {
      switch (this.state.callStatus) {
        case "failure":
        case "end":
        case "start": {
          return (
            StartConversationView({
              model: this.props.conversation, 
              notifications: this.props.notifications, 
              client: this.props.client}
            )
          );
        }
        case "pending": {
          return PendingConversationView({websocket: this._websocket});
        }
        case "connected": {
          return (
            sharedViews.ConversationView({
              sdk: this.props.sdk, 
              model: this.props.conversation, 
              video: {enabled: this.props.conversation.hasVideoStream("outgoing")}}
            )
          );
        }
        case "expired": {
          return (
            CallUrlExpiredView({helper: this.props.helper})
          );
        }
        default: {
          return HomeView(null)
        }
      }
    },

    



    _notifyError: function(error) {
      console.log(error);
      this.props.notifications.errorL10n("connection_error_see_console_notification");
      this.setState({callStatus: "end"});
    },

    





    _onPeerHungup: function() {
      this.props.notifications.warnL10n("peer_ended_conversation2");
      this.setState({callStatus: "end"});
    },

    


    _onNetworkDisconnected: function() {
      this.props.notifications.warnL10n("network_disconnected");
      this.setState({callStatus: "end"});
    },

    



    setupOutgoingCall: function() {
      var loopToken = this.props.conversation.get("loopToken");
      if (!loopToken) {
        this.props.notifications.errorL10n("missing_conversation_info");
        this.setState({callStatus: "failure"});
      } else {
        var callType = this.props.conversation.get("selectedCallType");

        this.props.client.requestCallInfo(this.props.conversation.get("loopToken"),
                                          callType, function(err, sessionData) {
          if (err) {
            switch (err.errno) {
              
              
              
              case 105:
                this.setState({callStatus: "expired"});
                break;
              default:
                this.props.notifications.errorL10n("missing_conversation_info");
                this.setState({callStatus: "failure"});
                break;
            }
            return;
          }
          this.props.conversation.outgoing(sessionData);
        }.bind(this));
      }
    },

    


    startCall: function() {
      var loopToken = this.props.conversation.get("loopToken");
      if (!loopToken) {
        this.props.notifications.errorL10n("missing_conversation_info");
        this.setState({callStatus: "failure"});
        return;
      }

      this._setupWebSocket();
      this.setState({callStatus: "pending"});
    },

    





    _setupWebSocket: function() {
      this._websocket = new loop.CallConnectionWebSocket({
        url: this.props.conversation.get("progressURL"),
        websocketToken: this.props.conversation.get("websocketToken"),
        callId: this.props.conversation.get("callId"),
      });
      this._websocket.promiseConnect().then(function() {
      }.bind(this), function() {
        
        
        this.props.notifications.errorL10n("cannot_start_call_session_not_ready");
        return;
      }.bind(this));

      this._websocket.on("progress", this._handleWebSocketProgress, this);
    },

    



    _checkConnected: function() {
      
      
      if (this.props.conversation.streamsConnected()) {
        this._websocket.mediaUp();
      }
    },

    



    _handleWebSocketProgress: function(progressData) {
      switch(progressData.state) {
        case "connecting": {
          
          this.setState({callStatus: "connected"});
          break;
        }
        case "terminated": {
          
          
          this._handleCallTerminated(progressData.reason);
          break;
        }
      }
    },

    




    _handleCallTerminated: function(reason) {
      this.setState({callStatus: "end"});
      
      if (reason !== "cancel") {
        
        
        this.props.notifications.errorL10n("call_timeout_notification_text");
      }
    },

    


    _endCall: function() {
      this.setState({callStatus: "end"});
    },
  });

  



  var WebappRootView = React.createClass({displayName: 'WebappRootView',
    propTypes: {
      client: React.PropTypes.instanceOf(loop.StandaloneClient).isRequired,
      conversation: React.PropTypes.instanceOf(sharedModels.ConversationModel)
                         .isRequired,
      helper: React.PropTypes.instanceOf(WebappHelper).isRequired,
      notifications: React.PropTypes.instanceOf(sharedModels.NotificationCollection)
                          .isRequired,
      sdk: React.PropTypes.object.isRequired
    },

    getInitialState: function() {
      return {
        unsupportedDevice: this.props.helper.isIOS(navigator.platform),
        unsupportedBrowser: !this.props.sdk.checkSystemRequirements(),
      };
    },

    render: function() {
      if (this.state.unsupportedDevice) {
        return UnsupportedDeviceView(null);
      } else if (this.state.unsupportedBrowser) {
        return UnsupportedBrowserView(null);
      } else if (this.props.conversation.get("loopToken")) {
        return (
          OutgoingConversationView({
             client: this.props.client, 
             conversation: this.props.conversation, 
             helper: this.props.helper, 
             notifications: this.props.notifications, 
             sdk: this.props.sdk}
          )
        );
      } else {
        return HomeView(null);
      }
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
    },

    locationHash: function() {
      return window.location.hash;
    }
  };

  


  function init() {
    var helper = new WebappHelper();
    var client = new loop.StandaloneClient({
      baseServerUrl: loop.config.serverUrl
    });
    var notifications = new sharedModels.NotificationCollection();
    var conversation = new sharedModels.ConversationModel({}, {
      sdk: OT
    });

    
    var locationHash = helper.locationHash();
    if (locationHash) {
      conversation.set("loopToken", locationHash.match(/\#call\/(.*)/)[1]);
    }

    React.renderComponent(WebappRootView({
      client: client, 
      conversation: conversation, 
      helper: helper, 
      notifications: notifications, 
      sdk: OT}
    ), document.querySelector("#main"));

    
    document.documentElement.lang = mozL10n.language.code;
    document.documentElement.dir = mozL10n.language.direction;
  }

  return {
    CallUrlExpiredView: CallUrlExpiredView,
    PendingConversationView: PendingConversationView,
    StartConversationView: StartConversationView,
    OutgoingConversationView: OutgoingConversationView,
    HomeView: HomeView,
    UnsupportedBrowserView: UnsupportedBrowserView,
    UnsupportedDeviceView: UnsupportedDeviceView,
    init: init,
    PromoteFirefoxView: PromoteFirefoxView,
    WebappHelper: WebappHelper,
    WebappRootView: WebappRootView
  };
})(jQuery, _, window.OT, navigator.mozL10n);
