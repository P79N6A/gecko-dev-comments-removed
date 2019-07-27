/** @jsx React.DOM */

/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/* global loop:true, React */
/* jshint newcap:false */

var loop = loop || {};
loop.webapp = (function($, _, OT, mozL10n) {
  "use strict";

  loop.config = loop.config || {};
  loop.config.serverUrl = loop.config.serverUrl || "http://localhost:5000";

  var sharedModels = loop.shared.models,
      sharedViews = loop.shared.views;

  /**
   * App router.
   * @type {loop.webapp.WebappRouter}
   */
  var router;

  /**
   * Homepage view.
   */
  var HomeView = React.createClass({
    render: function() {
      return (
        <p>{mozL10n.get("welcome")}</p>
      )
    }
  });

  /**
   * Unsupported Browsers view.
   */
  var UnsupportedBrowserView = React.createClass({
    render: function() {
      var useLatestFF = mozL10n.get("use_latest_firefox", {
        "firefoxBrandNameLink": React.renderComponentToStaticMarkup(
          <a target="_blank" href="https://www.mozilla.org/firefox/">Firefox</a>
        )
      });
      return (
        <div>
          <h2>{mozL10n.get("incompatible_browser")}</h2>
          <p>{mozL10n.get("powered_by_webrtc")}</p>
          <p dangerouslySetInnerHTML={{__html: useLatestFF}}></p>
        </div>
      );
    }
  });

  /**
   * Unsupported Device view.
   */
  var UnsupportedDeviceView = React.createClass({
    render: function() {
      return (
        <div>
          <h2>{mozL10n.get("incompatible_device")}</h2>
          <p>{mozL10n.get("sorry_device_unsupported")}</p>
          <p>{mozL10n.get("use_firefox_windows_mac_linux")}</p>
        </div>
      );
    }
  });

  /**
   * Firefox promotion interstitial. Will display only to non-Firefox users.
   */
  var PromoteFirefoxView = React.createClass({
    propTypes: {
      helper: React.PropTypes.object.isRequired
    },

    render: function() {
      if (this.props.helper.isFirefox(navigator.userAgent)) {
        return <div />;
      }
      return (
        <div className="promote-firefox">
          <h3>{mozL10n.get("promote_firefox_hello_heading")}</h3>
          <p>
            <a className="btn btn-large btn-accept"
               href="https://www.mozilla.org/firefox/">
              {mozL10n.get("get_firefox_button")}
            </a>
          </p>
        </div>
      );
    }
  });

  /**
   * Expired call URL view.
   */
  var CallUrlExpiredView = React.createClass({
    propTypes: {
      helper: React.PropTypes.object.isRequired
    },

    render: function() {
      /* jshint ignore:start */
      return (
        <div className="expired-url-info">
          <div className="info-panel">
            <div className="firefox-logo" />
            <h1>{mozL10n.get("call_url_unavailable_notification_heading")}</h1>
            <h4>{mozL10n.get("call_url_unavailable_notification_message2")}</h4>
          </div>
          <PromoteFirefoxView helper={this.props.helper} />
        </div>
      );
      /* jshint ignore:end */
    }
  });

  var ConversationBranding = React.createClass({
    render: function() {
      return (
        <h1 className="standalone-header-title">
          <strong>{mozL10n.get("brandShortname")}</strong> {mozL10n.get("clientShortname")}
        </h1>
      );
    }
  });

  var ConversationHeader = React.createClass({
    render: function() {
      var cx = React.addons.classSet;
      var conversationUrl = location.href;

      var urlCreationDateClasses = cx({
        "light-color-font": true,
        "call-url-date": true, /* Used as a handler in the tests */
        /*hidden until date is available*/
        "hide": !this.props.urlCreationDateString.length
      });

      var callUrlCreationDateString = mozL10n.get("call_url_creation_date_label", {
        "call_url_creation_date": this.props.urlCreationDateString
      });

      return (
        /* jshint ignore:start */
        <header className="standalone-header header-box container-box">
          <ConversationBranding />
          <div className="loop-logo" title="Firefox WebRTC! logo"></div>
          <h3 className="call-url">
            {conversationUrl}
          </h3>
          <h4 className={urlCreationDateClasses} >
            {callUrlCreationDateString}
          </h4>
        </header>
        /* jshint ignore:end */
      );
    }
  });

  var ConversationFooter = React.createClass({
    render: function() {
      return (
        <div className="standalone-footer container-box">
          <div title="Mozilla Logo" className="footer-logo"></div>
        </div>
      );
    }
  });

  var PendingConversationView = React.createClass({
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
        /* jshint ignore:start */
        <div className="container">
          <div className="container-box">
            <header className="pending-header header-box">
              <ConversationBranding />
            </header>

            <div id="cameraPreview"></div>

            <div id="messages"></div>

            <p className="standalone-btn-label">
              {callState}
            </p>

            <div className="btn-pending-cancel-group btn-group">
              <div className="flex-padding-1"></div>
              <button className="btn btn-large btn-cancel"
                      onClick={this._cancelOutgoingCall} >
                <span className="standalone-call-btn-text">
                  {mozL10n.get("initiate_call_cancel_button")}
                </span>
              </button>
              <div className="flex-padding-1"></div>
            </div>
          </div>

          <ConversationFooter />
        </div>
        /* jshint ignore:end */
      );
    }
  });

  /**
   * Conversation launcher view. A ConversationModel is associated and attached
   * as a `model` property.
   */
  var StartConversationView = React.createClass({
    /**
     * Constructor.
     *
     * Required options:
     * - {loop.shared.models.ConversationModel}    model    Conversation model.
     * - {loop.shared.models.NotificationCollection} notifications
     *
     */

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
      // XXX Check more tightly here when we start injecting window.loop.*
      notifications: React.PropTypes.object.isRequired,
      client: React.PropTypes.object.isRequired
    },

    componentDidMount: function() {
      // Listen for events & hide dropdown menu if user clicks away
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

    /**
     * Initiates the call.
     * Takes in a call type parameter "audio" or "audio-video" and returns
     * a function that initiates the call. React click handler requires a function
     * to be called when that event happenes.
     *
     * @param {string} User call type choice "audio" or "audio-video"
     */
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
        "terms_of_use_url": "<a target=_blank href='/legal/terms'>" +
          tos_link_name + "</a>",
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
        /* jshint ignore:start */
        <div className="container">
          <div className="container-box">

            <ConversationHeader
              urlCreationDateString={this.state.urlCreationDateString} />

            <p className="standalone-btn-label">
              {mozL10n.get("initiate_call_button_label2")}
            </p>

            <div id="messages"></div>

            <div className="btn-group">
              <div className="flex-padding-1"></div>
              <div className="standalone-btn-chevron-menu-group">
                <div className="btn-group-chevron">
                  <div className="btn-group">

                    <button className="btn btn-large btn-accept"
                            onClick={this._initiateOutgoingCall("audio-video")}
                            disabled={this.state.disableCallButton}
                            title={mozL10n.get("initiate_audio_video_call_tooltip2")} >
                      <span className="standalone-call-btn-text">
                        {mozL10n.get("initiate_audio_video_call_button2")}
                      </span>
                      <span className="standalone-call-btn-video-icon"></span>
                    </button>

                    <div className="btn-chevron"
                         onClick={this._toggleCallOptionsMenu}>
                    </div>

                  </div>

                  <ul className={dropdownMenuClasses}>
                    <li>
                      {/*
                       Button required for disabled state.
                       */}
                      <button className="start-audio-only-call"
                              onClick={this._initiateOutgoingCall("audio")}
                              disabled={this.state.disableCallButton} >
                        {mozL10n.get("initiate_audio_call_button2")}
                      </button>
                    </li>
                  </ul>

                </div>
              </div>
              <div className="flex-padding-1"></div>
            </div>

            <p className={tosClasses}
               dangerouslySetInnerHTML={{__html: tosHTML}}></p>
          </div>

          <ConversationFooter />
        </div>
        /* jshint ignore:end */
      );
    }
  });

  /**
   * This view manages the outgoing conversation views - from
   * call initiation through to the actual conversation and call end.
   *
   * At the moment, it does more than that, these parts need refactoring out.
   */
  var OutgoingConversationView = React.createClass({
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

    /**
     * Renders the conversation views.
     */
    render: function() {
      switch (this.state.callStatus) {
        case "failure":
        case "end":
        case "start": {
          return (
            <StartConversationView
              model={this.props.conversation}
              notifications={this.props.notifications}
              client={this.props.client}
            />
          );
        }
        case "pending": {
          return <PendingConversationView websocket={this._websocket} />;
        }
        case "connected": {
          return (
            <sharedViews.ConversationView
              sdk={this.props.sdk}
              model={this.props.conversation}
              video={{enabled: this.props.conversation.hasVideoStream("outgoing")}}
            />
          );
        }
        case "expired": {
          return (
            <CallUrlExpiredView helper={this.props.helper} />
          );
        }
        default: {
          return <HomeView />
        }
      }
    },

    /**
     * Notify the user that the connection was not possible
     * @param {{code: number, message: string}} error
     */
    _notifyError: function(error) {
      console.log(error);
      this.props.notifications.errorL10n("connection_error_see_console_notification");
      this.setState({callStatus: "end"});
    },

    /**
     * Peer hung up. Notifies the user and ends the call.
     *
     * Event properties:
     * - {String} connectionId: OT session id
     */
    _onPeerHungup: function() {
      this.props.notifications.warnL10n("peer_ended_conversation2");
      this.setState({callStatus: "end"});
    },

    /**
     * Network disconnected. Notifies the user and ends the call.
     */
    _onNetworkDisconnected: function() {
      this.props.notifications.warnL10n("network_disconnected");
      this.setState({callStatus: "end"});
    },

    /**
     * Starts the set up of a call, obtaining the required information from the
     * server.
     */
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
              // loop-server sends 404 + INVALID_TOKEN (errno 105) whenever a token is
              // missing OR expired; we treat this information as if the url is always
              // expired.
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

    /**
     * Actually starts the call.
     */
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

    /**
     * Used to set up the web socket connection and navigate to the
     * call view if appropriate.
     *
     * @param {string} loopToken The session token to use.
     */
    _setupWebSocket: function() {
      this._websocket = new loop.CallConnectionWebSocket({
        url: this.props.conversation.get("progressURL"),
        websocketToken: this.props.conversation.get("websocketToken"),
        callId: this.props.conversation.get("callId"),
      });
      this._websocket.promiseConnect().then(function() {
      }.bind(this), function() {
        // XXX Not the ideal response, but bug 1047410 will be replacing
        // this by better "call failed" UI.
        this.props.notifications.errorL10n("cannot_start_call_session_not_ready");
        return;
      }.bind(this));

      this._websocket.on("progress", this._handleWebSocketProgress, this);
    },

    /**
     * Checks if the streams have been connected, and notifies the
     * websocket that the media is now connected.
     */
    _checkConnected: function() {
      // Check we've had both local and remote streams connected before
      // sending the media up message.
      if (this.props.conversation.streamsConnected()) {
        this._websocket.mediaUp();
      }
    },

    /**
     * Used to receive websocket progress and to determine how to handle
     * it if appropraite.
     */
    _handleWebSocketProgress: function(progressData) {
      switch(progressData.state) {
        case "connecting": {
          // We just go straight to the connected view as the media gets set up.
          this.setState({callStatus: "connected"});
          break;
        }
        case "terminated": {
          // At the moment, we show the same text regardless
          // of the terminated reason.
          this._handleCallTerminated(progressData.reason);
          break;
        }
      }
    },

    /**
     * Handles call rejection.
     *
     * @param {String} reason The reason the call was terminated.
     */
    _handleCallTerminated: function(reason) {
      this.setState({callStatus: "end"});
      // For reasons other than cancel, display some notification text.
      if (reason !== "cancel") {
        // XXX This should really display the call failed view - bug 1046959
        // will implement this.
        this.props.notifications.errorL10n("call_timeout_notification_text");
      }
    },

    /**
     * Handles ending a call by resetting the view to the start state.
     */
    _endCall: function() {
      this.setState({callStatus: "end"});
    },
  });

  /**
   * Webapp Root View. This is the main, single, view that controls the display
   * of the webapp page.
   */
  var WebappRootView = React.createClass({
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
        return <UnsupportedDeviceView />;
      } else if (this.state.unsupportedBrowser) {
        return <UnsupportedBrowserView />;
      } else if (this.props.conversation.get("loopToken")) {
        return (
          <OutgoingConversationView
             client={this.props.client}
             conversation={this.props.conversation}
             helper={this.props.helper}
             notifications={this.props.notifications}
             sdk={this.props.sdk}
          />
        );
      } else {
        return <HomeView />;
      }
    }
  });

  /**
   * Local helpers.
   */
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

  /**
   * App initialization.
   */
  function init() {
    var helper = new WebappHelper();
    var client = new loop.StandaloneClient({
      baseServerUrl: loop.config.serverUrl
    });
    var notifications = new sharedModels.NotificationCollection();
    var conversation = new sharedModels.ConversationModel({}, {
      sdk: OT
    });

    // Obtain the loopToken and pass it to the conversation
    var locationHash = helper.locationHash();
    if (locationHash) {
      conversation.set("loopToken", locationHash.match(/\#call\/(.*)/)[1]);
    }

    React.renderComponent(<WebappRootView
      client={client}
      conversation={conversation}
      helper={helper}
      notifications={notifications}
      sdk={OT}
    />, document.querySelector("#main"));

    // Set the 'lang' and 'dir' attributes to <html> when the page is translated
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
