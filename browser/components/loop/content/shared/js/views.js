







var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.views = (function(_, OT, l10n) {
  "use strict";

  var sharedModels = loop.shared.models;
  var sharedMixins = loop.shared.mixins;

  








  var MediaControlButton = React.createClass({displayName: 'MediaControlButton',
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
        
        React.DOM.button({className: this._getClasses(), 
                title: this._getTitle(), 
                onClick: this.handleClick})
        
      );
    }
  });

  


  var ConversationToolbar = React.createClass({displayName: 'ConversationToolbar',
    getDefaultProps: function() {
      return {
        video: {enabled: true, visible: true},
        audio: {enabled: true, visible: true},
        enableHangup: true
      };
    },

    propTypes: {
      video: React.PropTypes.object.isRequired,
      audio: React.PropTypes.object.isRequired,
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
      var cx = React.addons.classSet;
      return (
        React.DOM.ul({className: "conversation-toolbar"}, 
          React.DOM.li({className: "conversation-toolbar-btn-box btn-hangup-entry"}, 
            React.DOM.button({className: "btn btn-hangup", onClick: this.handleClickHangup, 
                    title: l10n.get("hangup_button_title"), 
                    disabled: !this.props.enableHangup}, 
              this._getHangupButtonLabel()
            )
          ), 
          React.DOM.li({className: "conversation-toolbar-btn-box"}, 
            MediaControlButton({action: this.handleToggleVideo, 
                                enabled: this.props.video.enabled, 
                                visible: this.props.video.visible, 
                                scope: "local", type: "video"})
          ), 
          React.DOM.li({className: "conversation-toolbar-btn-box"}, 
            MediaControlButton({action: this.handleToggleAudio, 
                                enabled: this.props.audio.enabled, 
                                visible: this.props.audio.visible, 
                                scope: "local", type: "audio"})
          )
        )
      );
    }
  });

  


  var ConversationView = React.createClass({displayName: 'ConversationView',
    mixins: [Backbone.Events, sharedMixins.AudioMixin],

    propTypes: {
      sdk: React.PropTypes.object.isRequired,
      video: React.PropTypes.object,
      audio: React.PropTypes.object,
      initiate: React.PropTypes.bool
    },

    
    
    publisherConfig: {
      insertMode: "append",
      width: "100%",
      height: "100%",
      style: {
        audioLevelDisplayMode: "off",
        bugDisplayMode: "off",
        buttonDisplayMode: "off",
        nameDisplayMode: "off",
        videoDisabledDisplayMode: "off"
      }
    },

    getDefaultProps: function() {
      return {
        initiate: true,
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

    componentWillMount: function() {
      if (this.props.initiate) {
        this.publisherConfig.publishVideo = this.props.video.enabled;
      }
    },

    componentDidMount: function() {
      if (this.props.initiate) {
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

      





      window.addEventListener('orientationchange', this.updateVideoContainer);
      window.addEventListener('resize', this.updateVideoContainer);
    },

    updateVideoContainer: function() {
      var localStreamParent = document.querySelector('.local .OT_publisher');
      var remoteStreamParent = document.querySelector('.remote .OT_subscriber');
      if (localStreamParent) {
        localStreamParent.style.width = "100%";
      }
      if (remoteStreamParent) {
        remoteStreamParent.style.height = "100%";
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
      this.props.model.subscribe(event.stream, incoming, this.publisherConfig);
    },

    






    startPublishing: function(event) {
      var outgoing = this.getDOMNode().querySelector(".local");

      
      this.publisher = this.props.sdk.initPublisher(
        outgoing, this.publisherConfig);

      
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
        React.DOM.div({className: "video-layout-wrapper"}, 
          React.DOM.div({className: "conversation"}, 
            React.DOM.div({className: "media nested"}, 
              React.DOM.div({className: "video_wrapper remote_wrapper"}, 
                React.DOM.div({className: "video_inner remote"})
              ), 
              React.DOM.div({className: localStreamClasses})
            ), 
            ConversationToolbar({video: this.state.video, 
                                 audio: this.state.audio, 
                                 publishStream: this.publishStream, 
                                 hangup: this.hangup})
          )
        )
      );
      
    }
  });

  


  var NotificationView = React.createClass({displayName: 'NotificationView',
    mixins: [Backbone.Events],

    propTypes: {
      notification: React.PropTypes.object.isRequired,
      key: React.PropTypes.number.isRequired
    },

    render: function() {
      var notification = this.props.notification;
      return (
        React.DOM.div({className: "notificationContainer"}, 
          React.DOM.div({key: this.props.key, 
               className: "alert alert-" + notification.get("level")}, 
            React.DOM.span({className: "message"}, notification.get("message"))
          ), 
          React.DOM.div({className: "detailsBar details-" + notification.get("level"), 
               hidden: !notification.get("details")}, 
            React.DOM.button({className: "detailsButton btn-info", 
                    onClick: notification.get("detailsButtonCallback"), 
                    hidden: !notification.get("detailsButtonLabel") || !notification.get("detailsButtonCallback")}, 
              notification.get("detailsButtonLabel")
            ), 
            React.DOM.span({className: "details"}, notification.get("details"))
          )
        )
      );
    }
  });

  


  var NotificationListView = React.createClass({displayName: 'NotificationListView',
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
        React.DOM.div({className: "messages"}, 
          this.props.notifications.map(function(notification, key) {
            return NotificationView({key: key, notification: notification});
          })
        
        )
      );
    }
  });

  var Button = React.createClass({displayName: 'Button',
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
        React.DOM.button({onClick: this.props.onClick, 
                disabled: this.props.disabled, 
                id: this.props.htmlId, 
                className: cx(classObject)}, 
          React.DOM.span({className: "button-caption"}, this.props.caption), 
          this.props.children
        )
      );
    }
  });

  var ButtonGroup = React.createClass({displayName: 'ButtonGroup',
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
        React.DOM.div({className: cx(classObject)}, 
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
    NotificationListView: NotificationListView
  };
})(_, window.OT, navigator.mozL10n || document.mozL10n);
