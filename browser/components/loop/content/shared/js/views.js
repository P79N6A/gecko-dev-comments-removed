







var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.views = (function(_, l10n) {
  "use strict";

  var sharedActions = loop.shared.actions;
  var sharedModels = loop.shared.models;
  var sharedMixins = loop.shared.mixins;
  var SCREEN_SHARE_STATES = loop.shared.utils.SCREEN_SHARE_STATES;

  








  var MediaControlButton = React.createClass({displayName: "MediaControlButton",
    propTypes: {
      scope: React.PropTypes.string.isRequired,
      type: React.PropTypes.string.isRequired,
      action: React.PropTypes.func.isRequired,
      enabled: React.PropTypes.bool.isRequired,
      visible: React.PropTypes.bool.isRequired
    },

    getDefaultProps: function() {
      return {enabled: true, visible: true};
    },

    handleClick: function() {
      this.props.action();
    },

    _getClasses: function() {
      var cx = React.addons.classSet;
      
      var classesObj = {
        "btn": true,
        "media-control": true,
        "transparent-button": true,
        "local-media": this.props.scope === "local",
        "muted": !this.props.enabled,
        "hide": !this.props.visible
      };
      classesObj["btn-mute-" + this.props.type] = true;
      return cx(classesObj);
    },

    _getTitle: function(enabled) {
      var prefix = this.props.enabled ? "mute" : "unmute";
      var suffix = "button_title";
      var msgId = [prefix, this.props.scope, this.props.type, suffix].join("_");
      return l10n.get(msgId);
    },

    render: function() {
      return (
        
        React.createElement("button", {className: this._getClasses(), 
                title: this._getTitle(), 
                onClick: this.handleClick})
        
      );
    }
  });

  








  var ScreenShareControlButton = React.createClass({displayName: "ScreenShareControlButton",
    mixins: [sharedMixins.DropdownMenuMixin],

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      visible: React.PropTypes.bool.isRequired,
      state: React.PropTypes.string.isRequired,
    },

    getInitialState: function() {
      var os = loop.shared.utils.getOS();
      var osVersion = loop.shared.utils.getOSVersion();
      
      if ((os.indexOf("mac") > -1 && osVersion.major <= 10 && osVersion.minor <= 6) ||
          (os.indexOf("win") > -1 && osVersion.major <= 5 && osVersion.minor <= 2)) {
        return { windowSharingDisabled: true };
      }
      return { windowSharingDisabled: false };
    },

    handleClick: function() {
      if (this.props.state === SCREEN_SHARE_STATES.ACTIVE) {
        this.props.dispatcher.dispatch(
          new sharedActions.EndScreenShare({}));
      } else {
        this.toggleDropdownMenu();
      }
    },

    _startScreenShare: function(type) {
      this.props.dispatcher.dispatch(new sharedActions.StartScreenShare({
        type: type
      }));
    },

    _handleShareTabs: function() {
      this._startScreenShare("browser");
    },

    _handleShareWindows: function() {
      this._startScreenShare("window");
    },

    _getTitle: function() {
      var prefix = this.props.state === SCREEN_SHARE_STATES.ACTIVE ?
        "active" : "inactive";

      return l10n.get(prefix + "_screenshare_button_title");
    },

    render: function() {
      if (!this.props.visible) {
        return null;
      }

      var cx = React.addons.classSet;

      var isActive = this.props.state === SCREEN_SHARE_STATES.ACTIVE;
      var screenShareClasses = cx({
        "btn": true,
        "btn-screen-share": true,
        "transparent-button": true,
        "menu-showing": this.state.showMenu,
        "active": isActive,
        "disabled": this.props.state === SCREEN_SHARE_STATES.PENDING
      });
      var dropdownMenuClasses = cx({
        "native-dropdown-menu": true,
        "conversation-window-dropdown": true,
        "visually-hidden": !this.state.showMenu
      });
      var windowSharingClasses = cx({
        "disabled": this.state.windowSharingDisabled
      });

      return (
        React.createElement("div", null, 
          React.createElement("button", {className: screenShareClasses, 
                  onClick: this.handleClick, 
                  title: this._getTitle()}, 
            isActive ? null : React.createElement("span", {className: "chevron"})
          ), 
          React.createElement("ul", {ref: "menu", className: dropdownMenuClasses}, 
            React.createElement("li", {onClick: this._handleShareTabs}, 
              l10n.get("share_tabs_button_title")
            ), 
            React.createElement("li", {onClick: this._handleShareWindows, className: windowSharingClasses}, 
              l10n.get("share_windows_button_title")
            )
          )
        )
      );
    }
  });

  


  var ConversationToolbar = React.createClass({displayName: "ConversationToolbar",
    getDefaultProps: function() {
      return {
        video: {enabled: true, visible: true},
        audio: {enabled: true, visible: true},
        screenShare: {state: SCREEN_SHARE_STATES.INACTIVE, visible: false},
        enableHangup: true
      };
    },

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      video: React.PropTypes.object.isRequired,
      audio: React.PropTypes.object.isRequired,
      screenShare: React.PropTypes.object,
      hangup: React.PropTypes.func.isRequired,
      publishStream: React.PropTypes.func.isRequired,
      hangupButtonLabel: React.PropTypes.string,
      enableHangup: React.PropTypes.bool,
    },

    handleClickHangup: function() {
      this.props.hangup();
    },

    handleToggleVideo: function() {
      this.props.publishStream("video", !this.props.video.enabled);
    },

    handleToggleAudio: function() {
      this.props.publishStream("audio", !this.props.audio.enabled);
    },

    _getHangupButtonLabel: function() {
      return this.props.hangupButtonLabel || l10n.get("hangup_button_caption2");
    },

    render: function() {
      return (
        React.createElement("ul", {className: "conversation-toolbar"}, 
          React.createElement("li", {className: "conversation-toolbar-btn-box btn-hangup-entry"}, 
            React.createElement("button", {className: "btn btn-hangup", onClick: this.handleClickHangup, 
                    title: l10n.get("hangup_button_title"), 
                    disabled: !this.props.enableHangup}, 
              this._getHangupButtonLabel()
            )
          ), 
          React.createElement("li", {className: "conversation-toolbar-btn-box"}, 
            React.createElement(MediaControlButton, {action: this.handleToggleVideo, 
                                enabled: this.props.video.enabled, 
                                visible: this.props.video.visible, 
                                scope: "local", type: "video"})
          ), 
          React.createElement("li", {className: "conversation-toolbar-btn-box"}, 
            React.createElement(MediaControlButton, {action: this.handleToggleAudio, 
                                enabled: this.props.audio.enabled, 
                                visible: this.props.audio.visible, 
                                scope: "local", type: "audio"})
          ), 
          React.createElement("li", {className: "conversation-toolbar-btn-box btn-screen-share-entry"}, 
            React.createElement(ScreenShareControlButton, {dispatcher: this.props.dispatcher, 
                                      visible: this.props.screenShare.visible, 
                                      state: this.props.screenShare.state})
          )
        )
      );
    }
  });

  


  var ConversationView = React.createClass({displayName: "ConversationView",
    mixins: [
      Backbone.Events,
      sharedMixins.AudioMixin,
      sharedMixins.MediaSetupMixin
    ],

    propTypes: {
      sdk: React.PropTypes.object.isRequired,
      video: React.PropTypes.object,
      audio: React.PropTypes.object,
      initiate: React.PropTypes.bool,
      isDesktop: React.PropTypes.bool
    },

    getDefaultProps: function() {
      return {
        initiate: true,
        isDesktop: false,
        video: {enabled: true, visible: true},
        audio: {enabled: true, visible: true}
      };
    },

    getInitialState: function() {
      return {
        video: this.props.video,
        audio: this.props.audio
      };
    },

    componentDidMount: function() {
      if (this.props.initiate) {
        




        if (this.props.isDesktop &&
            !window.MediaStreamTrack.getSources) {
          
          
          
          window.MediaStreamTrack.getSources = function(callback) {
            callback([{kind: "audio"}, {kind: "video"}]);
          };
        }

        this.listenTo(this.props.sdk, "exception", this._handleSdkException.bind(this));

        this.listenTo(this.props.model, "session:connected",
                                        this._onSessionConnected);
        this.listenTo(this.props.model, "session:stream-created",
                                        this._streamCreated);
        this.listenTo(this.props.model, ["session:peer-hungup",
                                         "session:network-disconnected",
                                         "session:ended"].join(" "),
                                         this.stopPublishing);
        this.props.model.startSession();
      }
    },

    componentWillUnmount: function() {
      
      this.stopListening();
      this.hangup();
    },

    hangup: function() {
      this.stopPublishing();
      this.props.model.endSession();
    },

    _onSessionConnected: function(event) {
      this.startPublishing(event);
      this.play("connected");
    },

    









    _streamCreated: function(event) {
      var incoming = this.getDOMNode().querySelector(".remote");
      this.props.model.subscribe(event.stream, incoming,
        this.getDefaultPublisherConfig({
          publishVideo: this.props.video.enabled
        }));
    },

    






    _handleSdkException: function(event) {
      




      if (this.publisher &&
          event.code === OT.ExceptionCodes.UNABLE_TO_PUBLISH &&
          event.message === "GetUserMedia" &&
          this.state.video.enabled) {
        this.state.video.enabled = false;

        window.MediaStreamTrack.getSources = function(callback) {
          callback([{kind: "audio"}]);
        };

        this.stopListening(this.publisher);
        this.publisher.destroy();
        this.startPublishing();
      }
    },

    






    startPublishing: function(event) {
      var outgoing = this.getDOMNode().querySelector(".local");

      
      this.publisher = this.props.sdk.initPublisher(
        outgoing, this.getDefaultPublisherConfig({publishVideo: this.props.video.enabled}));

      
      this.listenTo(this.publisher, "accessDialogOpened accessDenied",
                    function(event) {
                      event.preventDefault();
                    });

      this.listenTo(this.publisher, "streamCreated", function(event) {
        this.setState({
          audio: {enabled: event.stream.hasAudio},
          video: {enabled: event.stream.hasVideo}
        });
      }.bind(this));

      this.listenTo(this.publisher, "streamDestroyed", function() {
        this.setState({
          audio: {enabled: false},
          video: {enabled: false}
        });
      }.bind(this));

      this.props.model.publish(this.publisher);
    },

    





    publishStream: function(type, enabled) {
      if (type === "audio") {
        this.publisher.publishAudio(enabled);
        this.setState({audio: {enabled: enabled}});
      } else {
        this.publisher.publishVideo(enabled);
        this.setState({video: {enabled: enabled}});
      }
    },

    


    stopPublishing: function() {
      if (this.publisher) {
        
        this.stopListening(this.publisher);

        this.props.model.session.unpublish(this.publisher);
      }
    },

    render: function() {
      var localStreamClasses = React.addons.classSet({
        local: true,
        "local-stream": true,
        "local-stream-audio": !this.state.video.enabled
      });
      
      return (
        React.createElement("div", {className: "video-layout-wrapper"}, 
          React.createElement("div", {className: "conversation in-call"}, 
            React.createElement("div", {className: "media nested"}, 
              React.createElement("div", {className: "video_wrapper remote_wrapper"}, 
                React.createElement("div", {className: "video_inner remote focus-stream"})
              ), 
              React.createElement("div", {className: localStreamClasses})
            ), 
            React.createElement(ConversationToolbar, {video: this.state.video, 
                                 audio: this.state.audio, 
                                 publishStream: this.publishStream, 
                                 hangup: this.hangup})
          )
        )
      );
      
    }
  });

  


  var NotificationView = React.createClass({displayName: "NotificationView",
    mixins: [Backbone.Events],

    propTypes: {
      notification: React.PropTypes.object.isRequired,
      key: React.PropTypes.number.isRequired
    },

    render: function() {
      var notification = this.props.notification;
      return (
        React.createElement("div", {className: "notificationContainer"}, 
          React.createElement("div", {key: this.props.key, 
               className: "alert alert-" + notification.get("level")}, 
            React.createElement("span", {className: "message"}, notification.get("message"))
          ), 
          React.createElement("div", {className: "detailsBar details-" + notification.get("level"), 
               hidden: !notification.get("details")}, 
            React.createElement("button", {className: "detailsButton btn-info", 
                    onClick: notification.get("detailsButtonCallback"), 
                    hidden: !notification.get("detailsButtonLabel") || !notification.get("detailsButtonCallback")}, 
              notification.get("detailsButtonLabel")
            ), 
            React.createElement("span", {className: "details"}, notification.get("details"))
          )
        )
      );
    }
  });

  


  var NotificationListView = React.createClass({displayName: "NotificationListView",
    mixins: [Backbone.Events, sharedMixins.DocumentVisibilityMixin],

    propTypes: {
      notifications: React.PropTypes.object.isRequired,
      clearOnDocumentHidden: React.PropTypes.bool
    },

    getDefaultProps: function() {
      return {clearOnDocumentHidden: false};
    },

    componentDidMount: function() {
      this.listenTo(this.props.notifications, "reset add remove", function() {
        this.forceUpdate();
      }.bind(this));
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.notifications);
    },

    




    onDocumentHidden: function() {
      if (this.props.clearOnDocumentHidden &&
          this.props.notifications.length > 0) {
        
        
        
        this.props.notifications.reset([], {silent: true});
        this.forceUpdate();
      }
    },

    render: function() {
      return (
        React.createElement("div", {className: "messages"}, 
          this.props.notifications.map(function(notification, key) {
            return React.createElement(NotificationView, {key: key, notification: notification});
          })
        
        )
      );
    }
  });

  var Button = React.createClass({displayName: "Button",
    propTypes: {
      caption: React.PropTypes.string.isRequired,
      onClick: React.PropTypes.func.isRequired,
      disabled: React.PropTypes.bool,
      additionalClass: React.PropTypes.string,
      htmlId: React.PropTypes.string,
    },

    getDefaultProps: function() {
      return {
        disabled: false,
        additionalClass: "",
        htmlId: "",
      };
    },

    render: function() {
      var cx = React.addons.classSet;
      var classObject = { button: true, disabled: this.props.disabled };
      if (this.props.additionalClass) {
        classObject[this.props.additionalClass] = true;
      }
      return (
        React.createElement("button", {onClick: this.props.onClick, 
                disabled: this.props.disabled, 
                id: this.props.htmlId, 
                className: cx(classObject)}, 
          React.createElement("span", {className: "button-caption"}, this.props.caption), 
          this.props.children
        )
      );
    }
  });

  var ButtonGroup = React.createClass({displayName: "ButtonGroup",
    PropTypes: {
      additionalClass: React.PropTypes.string
    },

    getDefaultProps: function() {
      return {
        additionalClass: "",
      };
    },

    render: function() {
      var cx = React.addons.classSet;
      var classObject = { "button-group": true };
      if (this.props.additionalClass) {
        classObject[this.props.additionalClass] = true;
      }
      return (
        React.createElement("div", {className: cx(classObject)}, 
          this.props.children
        )
      );
    }
  });

  return {
    Button: Button,
    ButtonGroup: ButtonGroup,
    ConversationView: ConversationView,
    ConversationToolbar: ConversationToolbar,
    MediaControlButton: MediaControlButton,
    ScreenShareControlButton: ScreenShareControlButton,
    NotificationListView: NotificationListView
  };
})(_, navigator.mozL10n || document.mozL10n);
