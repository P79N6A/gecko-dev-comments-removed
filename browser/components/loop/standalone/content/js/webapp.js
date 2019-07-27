








var loop = loop || {};
loop.webapp = (function($, _, OT, mozL10n) {
  "use strict";

  loop.config = loop.config || {};
  loop.config.serverUrl = loop.config.serverUrl || "http://localhost:5000";

  var sharedMixins = loop.shared.mixins;
  var sharedModels = loop.shared.models;
  var sharedViews = loop.shared.views;
  var sharedUtils = loop.shared.utils;

  


  var HomeView = React.createClass({displayName: 'HomeView',
    render: function() {
      return (
        React.DOM.p(null, mozL10n.get("welcome", {clientShortname: mozL10n.get("clientShortname2")}))
      );
    }
  });

  


  var UnsupportedBrowserView = React.createClass({displayName: 'UnsupportedBrowserView',
    render: function() {
      var useLatestFF = mozL10n.get("use_latest_firefox", {
        "firefoxBrandNameLink": React.renderComponentToStaticMarkup(
          React.DOM.a({target: "_blank", href: mozL10n.get("brand_website")}, mozL10n.get("brandShortname"))
        )
      });
      return (
        React.DOM.div(null, 
          React.DOM.h2(null, mozL10n.get("incompatible_browser")), 
          React.DOM.p(null, mozL10n.get("powered_by_webrtc", {clientShortname: mozL10n.get("clientShortname2")})), 
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
          React.DOM.p(null, mozL10n.get("sorry_device_unsupported", {clientShortname: mozL10n.get("clientShortname2")})), 
          React.DOM.p(null, mozL10n.get("use_firefox_windows_mac_linux", {brandShortname: mozL10n.get("brandShortname")}))
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
          React.DOM.h3(null, mozL10n.get("promote_firefox_hello_heading", {brandShortname: mozL10n.get("brandShortname")})), 
          React.DOM.p(null, 
            React.DOM.a({className: "btn btn-large btn-accept", 
               href: mozL10n.get("brand_website")}, 
              mozL10n.get("get_firefox_button", {brandShortname: mozL10n.get("brandShortname")})
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
          React.DOM.strong(null, mozL10n.get("brandShortname")), 
          mozL10n.get("clientShortname2")
        )
      );
    }
  });

  







  var FxOSHiddenMarketplace = React.createClass({displayName: 'FxOSHiddenMarketplace',
    render: function() {
      return React.DOM.iframe({id: "marketplace", src: this.props.marketplaceSrc, hidden: true});
    },

    componentDidUpdate: function() {
      
      if (this.props.onMarketplaceMessage) {
        
        
        
        window.addEventListener("message", this.props.onMarketplaceMessage);
      }
    }
  });

  var FxOSConversationModel = Backbone.Model.extend({
    setupOutgoingCall: function() {
      
      
      
      var request = new MozActivity({
        name: "loop-call",
        data: {
          type: "loop/token",
          token: this.get("loopToken"),
          callerId: this.get("callerId"),
          callType: this.get("callType")
        }
      });

      request.onsuccess = function() {};

      request.onerror = (function(event) {
        if (event.target.error.name !== "NO_PROVIDER") {
          console.error ("Unexpected " + event.target.error.name);
          this.trigger("session:error", "fxos_app_needed", {
            fxosAppName: loop.config.fxosApp.name
          });
          return;
        }
        this.trigger("fxos:app-needed");
      }).bind(this);
    },

    onMarketplaceMessage: function(event) {
      var message = event.data;
      switch (message.name) {
        case "loaded":
          var marketplace = window.document.getElementById("marketplace");
          
          
          
          marketplace.contentWindow.postMessage({
            "name": "install-package",
            "data": {
              "product": {
                "name": loop.config.fxosApp.name,
                "manifest_url": loop.config.fxosApp.manifestUrl,
                "is_packaged": true
              }
            }
          }, "*");
          break;
        case "install-package":
          window.removeEventListener("message", this.onMarketplaceMessage);
          if (message.error) {
            console.error(message.error.error);
            this.trigger("session:error", "fxos_app_needed", {
              fxosAppName: loop.config.fxosApp.name
            });
            return;
          }
          
          
          this.setupOutgoingCall();
          break;
      }
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
          React.DOM.div({className: "loop-logo", 
               title: mozL10n.get("client_alttext",
                                  {clientShortname: mozL10n.get("clientShortname2")})}), 
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
          React.DOM.div({title: mozL10n.get("vendor_alttext",
                                  {vendorShortname: mozL10n.get("vendorShortname")}), 
               className: "footer-logo"})
        )
      );
    }
  });

  var PendingConversationView = React.createClass({displayName: 'PendingConversationView',
    getInitialState: function() {
      return {
        callState: this.props.callState || "connecting"
      };
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

  var InitiateCallButton = React.createClass({displayName: 'InitiateCallButton',
    mixins: [sharedMixins.DropdownMenuMixin],

    propTypes: {
      caption: React.PropTypes.string.isRequired,
      startCall: React.PropTypes.func.isRequired,
      disabled: React.PropTypes.bool
    },

    getDefaultProps: function() {
      return {disabled: false};
    },

    render: function() {
      var dropdownMenuClasses = React.addons.classSet({
        "native-dropdown-large-parent": true,
        "standalone-dropdown-menu": true,
        "visually-hidden": !this.state.showMenu
      });
      var chevronClasses = React.addons.classSet({
        "btn-chevron": true,
        "disabled": this.props.disabled
      });
      return (
        React.DOM.div({className: "standalone-btn-chevron-menu-group"}, 
          React.DOM.div({className: "btn-group-chevron"}, 
            React.DOM.div({className: "btn-group"}, 
              React.DOM.button({className: "btn btn-large btn-accept", 
                      onClick: this.props.startCall("audio-video"), 
                      disabled: this.props.disabled, 
                      title: mozL10n.get("initiate_audio_video_call_tooltip2")}, 
                React.DOM.span({className: "standalone-call-btn-text"}, 
                  this.props.caption
                ), 
                React.DOM.span({className: "standalone-call-btn-video-icon"})
              ), 
              React.DOM.div({className: chevronClasses, 
                   onClick: this.toggleDropdownMenu}
              )
            ), 
            React.DOM.ul({className: dropdownMenuClasses}, 
              React.DOM.li(null, 
                React.DOM.button({className: "start-audio-only-call", 
                        onClick: this.props.startCall("audio"), 
                        disabled: this.props.disabled}, 
                  mozL10n.get("initiate_audio_call_button2")
                )
              )
            )
          )
        )
      );
    }
  });

  


  var InitiateConversationView = React.createClass({displayName: 'InitiateConversationView',
    mixins: [Backbone.Events],

    propTypes: {
      conversation: React.PropTypes.oneOfType([
                      React.PropTypes.instanceOf(sharedModels.ConversationModel),
                      React.PropTypes.instanceOf(FxOSConversationModel)
                    ]).isRequired,
      
      notifications: React.PropTypes.object.isRequired,
      client: React.PropTypes.object.isRequired,
      title: React.PropTypes.string.isRequired,
      callButtonLabel: React.PropTypes.string.isRequired
    },

    getInitialState: function() {
      return {
        urlCreationDateString: '',
        disableCallButton: false
      };
    },

    componentDidMount: function() {
      this.listenTo(this.props.conversation,
                    "session:error", this._onSessionError);
      this.listenTo(this.props.conversation,
                    "fxos:app-needed", this._onFxOSAppNeeded);
      this.props.client.requestCallUrlInfo(
        this.props.conversation.get("loopToken"),
        this._setConversationTimestamp);
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.conversation);
      localStorage.setItem("has-seen-tos", "true");
    },

    _onSessionError: function(error, l10nProps) {
      var errorL10n = error || "unable_retrieve_call_info";
      this.props.notifications.errorL10n(errorL10n, l10nProps);
      console.error(errorL10n);
    },

    _onFxOSAppNeeded: function() {
      this.setState({
        marketplaceSrc: loop.config.marketplaceUrl,
        onMarketplaceMessage: this.props.conversation.onMarketplaceMessage.bind(
          this.props.conversation
        )
      });
     },

    







    startCall: function(callType) {
      return function() {
        this.props.conversation.setupOutgoingCall(callType);
        this.setState({disableCallButton: true});
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

    render: function() {
      var tosLinkName = mozL10n.get("terms_of_use_link_text");
      var privacyNoticeName = mozL10n.get("privacy_notice_link_text");

      var tosHTML = mozL10n.get("legal_text_and_links", {
        "terms_of_use_url": "<a target=_blank href='" +
          mozL10n.get("legal_website") + "'>" +
          tosLinkName + "</a>",
        "privacy_notice_url": "<a target=_blank href='" +
          mozL10n.get("privacy_website") + "'>" + privacyNoticeName + "</a>"
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
              this.props.title
            ), 

            React.DOM.div({id: "messages"}), 

            React.DOM.div({className: "btn-group"}, 
              React.DOM.div({className: "flex-padding-1"}), 
              InitiateCallButton({
                caption: this.props.callButtonLabel, 
                disabled: this.state.disableCallButton, 
                startCall: this.startCall}
              ), 
              React.DOM.div({className: "flex-padding-1"})
            ), 

            React.DOM.p({className: tosClasses, 
               dangerouslySetInnerHTML: {__html: tosHTML}})
          ), 

          FxOSHiddenMarketplace({
            marketplaceSrc: this.state.marketplaceSrc, 
            onMarketplaceMessage: this.state.onMarketplaceMessage}), 

          ConversationFooter(null)
        )
      );
    }
  });

  


  var EndedConversationView = React.createClass({displayName: 'EndedConversationView',
    propTypes: {
      conversation: React.PropTypes.instanceOf(sharedModels.ConversationModel)
                         .isRequired,
      sdk: React.PropTypes.object.isRequired,
      feedbackApiClient: React.PropTypes.object.isRequired,
      onAfterFeedbackReceived: React.PropTypes.func.isRequired
    },

    render: function() {
      return (
        React.DOM.div({className: "ended-conversation"}, 
          sharedViews.FeedbackView({
            feedbackApiClient: this.props.feedbackApiClient, 
            onAfterFeedbackReceived: this.props.onAfterFeedbackReceived}
          ), 
          sharedViews.ConversationView({
            initiate: false, 
            sdk: this.props.sdk, 
            model: this.props.conversation, 
            audio: {enabled: false, visible: false}, 
            video: {enabled: false, visible: false}}
          )
        )
      );
    }
  });

  var StartConversationView = React.createClass({displayName: 'StartConversationView',
    render: function() {
      return this.transferPropsTo(
        InitiateConversationView({
          title: mozL10n.get("initiate_call_button_label2"), 
          callButtonLabel: mozL10n.get("initiate_audio_video_call_button2")})
      );
    }
  });

  var FailedConversationView = React.createClass({displayName: 'FailedConversationView',
    render: function() {
      return this.transferPropsTo(
        InitiateConversationView({
          title: mozL10n.get("call_failed_title"), 
          callButtonLabel: mozL10n.get("retry_call_button")})
      );
    }
  });

  





  var OutgoingConversationView = React.createClass({displayName: 'OutgoingConversationView',
    propTypes: {
      client: React.PropTypes.instanceOf(loop.StandaloneClient).isRequired,
      conversation: React.PropTypes.oneOfType([
        React.PropTypes.instanceOf(sharedModels.ConversationModel),
        React.PropTypes.instanceOf(FxOSConversationModel)
      ]).isRequired,
      helper: React.PropTypes.instanceOf(sharedUtils.Helper).isRequired,
      notifications: React.PropTypes.instanceOf(sharedModels.NotificationCollection)
                          .isRequired,
      sdk: React.PropTypes.object.isRequired,
      feedbackApiClient: React.PropTypes.object.isRequired
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

    shouldComponentUpdate: function(nextProps, nextState) {
      
      return nextState.callStatus !== this.state.callStatus;
    },

    callStatusSwitcher: function(status) {
      return function() {
        this.setState({callStatus: status});
      }.bind(this);
    },

    


    render: function() {
      switch (this.state.callStatus) {
        case "start": {
          return (
            StartConversationView({
              conversation: this.props.conversation, 
              notifications: this.props.notifications, 
              client: this.props.client}
            )
          );
        }
        case "failure": {
          return (
            FailedConversationView({
              conversation: this.props.conversation, 
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
              initiate: true, 
              sdk: this.props.sdk, 
              model: this.props.conversation, 
              video: {enabled: this.props.conversation.hasVideoStream("outgoing")}}
            )
          );
        }
        case "end": {
          return (
            EndedConversationView({
              sdk: this.props.sdk, 
              conversation: this.props.conversation, 
              feedbackApiClient: this.props.feedbackApiClient, 
              onAfterFeedbackReceived: this.callStatusSwitcher("start")}
            )
          );
        }
        case "expired": {
          return (
            CallUrlExpiredView({helper: this.props.helper})
          );
        }
        default: {
          return HomeView(null);
        }
      }
    },

    



    _notifyError: function(error) {
      console.error(error);
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
      if (reason === "cancel") {
        this.setState({callStatus: "start"});
        return;
      }
      
      this.props.notifications.errorL10n("call_timeout_notification_text");
      this.setState({callStatus: "failure"});
    },

    


    _endCall: function() {
      this.setState({callStatus: "end"});
    },
  });

  



  var WebappRootView = React.createClass({displayName: 'WebappRootView',
    propTypes: {
      client: React.PropTypes.instanceOf(loop.StandaloneClient).isRequired,
      conversation: React.PropTypes.oneOfType([
        React.PropTypes.instanceOf(sharedModels.ConversationModel),
        React.PropTypes.instanceOf(FxOSConversationModel)
      ]).isRequired,
      helper: React.PropTypes.instanceOf(sharedUtils.Helper).isRequired,
      notifications: React.PropTypes.instanceOf(sharedModels.NotificationCollection)
                          .isRequired,
      sdk: React.PropTypes.object.isRequired,
      feedbackApiClient: React.PropTypes.object.isRequired
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
             sdk: this.props.sdk, 
             feedbackApiClient: this.props.feedbackApiClient}
          )
        );
      } else {
        return HomeView(null);
      }
    }
  });

  


  function init() {
    var helper = new sharedUtils.Helper();
    var client = new loop.StandaloneClient({
      baseServerUrl: loop.config.serverUrl
    });
    var notifications = new sharedModels.NotificationCollection();
    var conversation
    if (helper.isFirefoxOS(navigator.userAgent)) {
      conversation = new FxOSConversationModel();
    } else {
      conversation = new sharedModels.ConversationModel({}, {
        sdk: OT
      });
    }

    var feedbackApiClient = new loop.FeedbackAPIClient(
      loop.config.feedbackApiUrl, {
        product: loop.config.feedbackProductName,
        user_agent: navigator.userAgent,
        url: document.location.origin
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
      sdk: OT, 
      feedbackApiClient: feedbackApiClient}
    ), document.querySelector("#main"));

    
    document.documentElement.lang = mozL10n.language.code;
    document.documentElement.dir = mozL10n.language.direction;
    document.title = mozL10n.get("clientShortname2");
  }

  return {
    CallUrlExpiredView: CallUrlExpiredView,
    PendingConversationView: PendingConversationView,
    StartConversationView: StartConversationView,
    FailedConversationView: FailedConversationView,
    OutgoingConversationView: OutgoingConversationView,
    EndedConversationView: EndedConversationView,
    HomeView: HomeView,
    UnsupportedBrowserView: UnsupportedBrowserView,
    UnsupportedDeviceView: UnsupportedDeviceView,
    init: init,
    PromoteFirefoxView: PromoteFirefoxView,
    WebappRootView: WebappRootView,
    FxOSConversationModel: FxOSConversationModel
  };
})(jQuery, _, window.OT, navigator.mozL10n);
