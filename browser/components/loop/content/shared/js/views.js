



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
      action: React.PropTypes.func.isRequired,
      enabled: React.PropTypes.bool.isRequired,
      scope: React.PropTypes.string.isRequired,
      type: React.PropTypes.string.isRequired,
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
                onClick: this.handleClick, 
                title: this._getTitle()})
      );
    }
  });

  








  var ScreenShareControlButton = React.createClass({displayName: "ScreenShareControlButton",
    mixins: [sharedMixins.DropdownMenuMixin()],

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      state: React.PropTypes.string.isRequired,
      visible: React.PropTypes.bool.isRequired
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
        "hide": !this.state.showMenu,
        "visually-hidden": true
      });
      var windowSharingClasses = cx({
        "disabled": this.state.windowSharingDisabled
      });

      return (
        React.createElement("div", null, 
          React.createElement("button", {className: screenShareClasses, 
                  onClick: this.handleClick, 
                  ref: "menu-button", 
                  title: this._getTitle()}, 
            isActive ? null : React.createElement("span", {className: "chevron"})
          ), 
          React.createElement("ul", {className: dropdownMenuClasses, ref: "menu"}, 
            React.createElement("li", {onClick: this._handleShareTabs}, 
              l10n.get("share_tabs_button_title2")
            ), 
            React.createElement("li", {className: windowSharingClasses, onClick: this._handleShareWindows}, 
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
      audio: React.PropTypes.object.isRequired,
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      enableHangup: React.PropTypes.bool,
      hangup: React.PropTypes.func.isRequired,
      hangupButtonLabel: React.PropTypes.string,
      publishStream: React.PropTypes.func.isRequired,
      screenShare: React.PropTypes.object,
      video: React.PropTypes.object.isRequired
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
            React.createElement("button", {className: "btn btn-hangup", 
                    disabled: !this.props.enableHangup, 
                    onClick: this.handleClickHangup, 
                    title: l10n.get("hangup_button_title")}, 
              this._getHangupButtonLabel()
            )
          ), 
          React.createElement("li", {className: "conversation-toolbar-btn-box"}, 
            React.createElement(MediaControlButton, {action: this.handleToggleVideo, 
                                enabled: this.props.video.enabled, 
                                scope: "local", type: "video", 
                                visible: this.props.video.visible})
          ), 
          React.createElement("li", {className: "conversation-toolbar-btn-box"}, 
            React.createElement(MediaControlButton, {action: this.handleToggleAudio, 
                                enabled: this.props.audio.enabled, 
                                scope: "local", type: "audio", 
                                visible: this.props.audio.visible})
          ), 
          React.createElement("li", {className: "conversation-toolbar-btn-box btn-screen-share-entry"}, 
            React.createElement(ScreenShareControlButton, {dispatcher: this.props.dispatcher, 
                                      state: this.props.screenShare.state, 
                                      visible: this.props.screenShare.visible})
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
      audio: React.PropTypes.object,
      initiate: React.PropTypes.bool,
      isDesktop: React.PropTypes.bool,
      model: React.PropTypes.object.isRequired,
      sdk: React.PropTypes.object.isRequired,
      video: React.PropTypes.object
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
                    function(ev) {
                      ev.preventDefault();
                    });

      this.listenTo(this.publisher, "streamCreated", function(ev) {
        this.setState({
          audio: {enabled: ev.stream.hasAudio},
          video: {enabled: ev.stream.hasVideo}
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
            React.createElement(ConversationToolbar, {
              audio: this.state.audio, 
              hangup: this.hangup, 
              publishStream: this.publishStream, 
              video: this.state.video})
          )
        )
      );
    }
  });

  


  var NotificationView = React.createClass({displayName: "NotificationView",
    mixins: [Backbone.Events],

    propTypes: {
      key: React.PropTypes.number.isRequired,
      notification: React.PropTypes.object.isRequired
    },

    render: function() {
      var notification = this.props.notification;
      return (
        React.createElement("div", {className: "notificationContainer"}, 
          React.createElement("div", {className: "alert alert-" + notification.get("level"), 
            key: this.props.key}, 
            React.createElement("span", {className: "message"}, notification.get("message"))
          ), 
          React.createElement("div", {className: "detailsBar details-" + notification.get("level"), 
               hidden: !notification.get("details")}, 
            React.createElement("button", {className: "detailsButton btn-info", 
                    hidden: !notification.get("detailsButtonLabel") || !notification.get("detailsButtonCallback"), 
                    onClick: notification.get("detailsButtonCallback")}, 
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
      clearOnDocumentHidden: React.PropTypes.bool,
      notifications: React.PropTypes.object.isRequired
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
      additionalClass: React.PropTypes.string,
      caption: React.PropTypes.string.isRequired,
      children: React.PropTypes.element,
      disabled: React.PropTypes.bool,
      htmlId: React.PropTypes.string,
      onClick: React.PropTypes.func.isRequired
    },

    getDefaultProps: function() {
      return {
        disabled: false,
        additionalClass: "",
        htmlId: ""
      };
    },

    render: function() {
      var cx = React.addons.classSet;
      var classObject = { button: true, disabled: this.props.disabled };
      if (this.props.additionalClass) {
        classObject[this.props.additionalClass] = true;
      }
      return (
        React.createElement("button", {className: cx(classObject), 
                disabled: this.props.disabled, 
                id: this.props.htmlId, 
                onClick: this.props.onClick}, 
          React.createElement("span", {className: "button-caption"}, this.props.caption), 
          this.props.children
        )
      );
    }
  });

  var ButtonGroup = React.createClass({displayName: "ButtonGroup",
    propTypes: {
      additionalClass: React.PropTypes.string,
      children: React.PropTypes.oneOfType([
        React.PropTypes.element,
        React.PropTypes.arrayOf(React.PropTypes.element)
      ])
    },

    getDefaultProps: function() {
      return {
        additionalClass: ""
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

  var Checkbox = React.createClass({displayName: "Checkbox",
    propTypes: {
      additionalClass: React.PropTypes.string,
      checked: React.PropTypes.bool,
      disabled: React.PropTypes.bool,
      label: React.PropTypes.string,
      onChange: React.PropTypes.func.isRequired,
      
      
      value: React.PropTypes.string
    },

    getDefaultProps: function() {
      return {
        additionalClass: "",
        checked: false,
        disabled: false,
        label: null,
        value: ""
      };
    },

    componentWillReceiveProps: function(nextProps) {
      
      
      if (this.props.checked !== nextProps.checked &&
          this.state.checked !== nextProps.checked) {
        this.setState({ checked: nextProps.checked });
      }
    },

    getInitialState: function() {
      return {
        checked: this.props.checked,
        value: this.props.checked ? this.props.value : ""
      };
    },

    _handleClick: function(event) {
      event.preventDefault();

      var newState = {
        checked: !this.state.checked,
        value: this.state.checked ? "" : this.props.value
      };
      this.setState(newState);
      this.props.onChange(newState);
    },

    render: function() {
      var cx = React.addons.classSet;
      var wrapperClasses = {
        "checkbox-wrapper": true,
        disabled: this.props.disabled
      };
      var checkClasses = {
        checkbox: true,
        checked: this.state.checked,
        disabled: this.props.disabled
      };
      if (this.props.additionalClass) {
        wrapperClasses[this.props.additionalClass] = true;
      }
      return (
        React.createElement("div", {className: cx(wrapperClasses), 
             disabled: this.props.disabled, 
             onClick: this._handleClick}, 
          React.createElement("div", {className: cx(checkClasses)}), 
          this.props.label ?
            React.createElement("label", null, this.props.label) :
            null
        )
      );
    }
  });

  


  var AvatarView = React.createClass({displayName: "AvatarView",
    mixins: [React.addons.PureRenderMixin],

    render: function() {
        return React.createElement("div", {className: "avatar"});
    }
  });

  


  var LoadingView = React.createClass({displayName: "LoadingView",
    mixins: [React.addons.PureRenderMixin],

    render: function() {
        return (
          React.createElement("div", {className: "loading-background"}, 
            React.createElement("div", {className: "loading-stream"})
          )
        );
    }
  });

  















  var ContextUrlView = React.createClass({displayName: "ContextUrlView",
    mixins: [React.addons.PureRenderMixin],

    propTypes: {
      allowClick: React.PropTypes.bool.isRequired,
      description: React.PropTypes.string.isRequired,
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher),
      showContextTitle: React.PropTypes.bool.isRequired,
      thumbnail: React.PropTypes.string,
      url: React.PropTypes.string,
      useDesktopPaths: React.PropTypes.bool.isRequired
    },

    


    handleLinkClick: function() {
      if (!this.props.allowClick) {
        return;
      }

      this.props.dispatcher.dispatch(new sharedActions.RecordClick({
        linkInfo: "Shared URL"
      }));
    },

    


    renderContextTitle: function() {
      if (!this.props.showContextTitle) {
        return null;
      }

      return React.createElement("p", null, l10n.get("context_inroom_label"));
    },

    render: function() {
      var hostname;

      try {
        hostname = new URL(this.props.url).hostname;
      } catch (ex) {
        return null;
      }

      var thumbnail = this.props.thumbnail;

      if (!thumbnail) {
        thumbnail = this.props.useDesktopPaths ?
          "loop/shared/img/icons-16x16.svg#globe" :
          "shared/img/icons-16x16.svg#globe";
      }

      return (
        React.createElement("div", {className: "context-content"}, 
          this.renderContextTitle(), 
          React.createElement("div", {className: "context-wrapper"}, 
            React.createElement("img", {className: "context-preview", src: thumbnail}), 
            React.createElement("span", {className: "context-description"}, 
              this.props.description, 
              React.createElement("a", {className: "context-url", 
                 href: this.props.allowClick ? this.props.url : null, 
                 onClick: this.handleLinkClick, 
                 rel: "noreferrer", 
                 target: "_blank"}, hostname)
            )
          )
        )
      );
    }
  });

  



  var MediaView = React.createClass({displayName: "MediaView",
    
    
    mixins: [React.addons.PureRenderMixin],

    propTypes: {
      displayAvatar: React.PropTypes.bool.isRequired,
      isLoading: React.PropTypes.bool.isRequired,
      mediaType: React.PropTypes.string.isRequired,
      posterUrl: React.PropTypes.string,
      
      srcVideoObject: React.PropTypes.object
    },

    componentDidMount: function() {
      if (!this.props.displayAvatar) {
        this.attachVideo(this.props.srcVideoObject);
      }
    },

    componentDidUpdate: function() {
      if (!this.props.displayAvatar) {
        this.attachVideo(this.props.srcVideoObject);
      }
    },

    









    attachVideo: function(srcVideoObject) {
      if (!srcVideoObject) {
        
        return;
      }

      var videoElement = this.getDOMNode();

      if (videoElement.tagName.toLowerCase() !== "video") {
        
        return;
      }

      
      var attrName = "";
      if ("srcObject" in videoElement) {
        
        attrName = "srcObject";
      } else if ("mozSrcObject" in videoElement) {
        
        attrName = "mozSrcObject";
      } else if ("src" in videoElement) {
        
        attrName = "src";
      } else {
        console.error("Error attaching stream to element - no supported" +
                      "attribute found");
        return;
      }

      
      if (videoElement[attrName] !== srcVideoObject[attrName]) {
        videoElement[attrName] = srcVideoObject[attrName];
      }
      videoElement.play();
    },

    render: function() {
      if (this.props.isLoading) {
        return React.createElement(LoadingView, null);
      }

      if (this.props.displayAvatar) {
        return React.createElement(AvatarView, null);
      }

      if (!this.props.srcVideoObject && !this.props.posterUrl) {
        return React.createElement("div", {className: "no-video"});
      }

      var optionalPoster = {};
      if (this.props.posterUrl) {
        optionalPoster.poster = this.props.posterUrl;
      }

      
      
      
      
      
      
      
      
      
      return (
        React.createElement("video", React.__spread({},  optionalPoster, 
               {className: this.props.mediaType + "-video", 
               muted: true}))
      );
    }
  });

  return {
    AvatarView: AvatarView,
    Button: Button,
    ButtonGroup: ButtonGroup,
    Checkbox: Checkbox,
    ContextUrlView: ContextUrlView,
    ConversationView: ConversationView,
    ConversationToolbar: ConversationToolbar,
    MediaControlButton: MediaControlButton,
    MediaView: MediaView,
    LoadingView: LoadingView,
    ScreenShareControlButton: ScreenShareControlButton,
    NotificationListView: NotificationListView
  };
})(_, navigator.mozL10n || document.mozL10n);
