








var loop = loop || {};
loop.roomViews = (function(mozL10n) {
  "use strict";

  var sharedActions = loop.shared.actions;
  var sharedMixins = loop.shared.mixins;
  var ROOM_STATES = loop.store.ROOM_STATES;
  var sharedViews = loop.shared.views;

  



  var ActiveRoomStoreMixin = {
    mixins: [Backbone.Events],

    propTypes: {
      roomStore: React.PropTypes.instanceOf(loop.store.RoomStore).isRequired
    },

    componentWillMount: function() {
      this.listenTo(this.props.roomStore, "change:activeRoom",
                    this._onActiveRoomStateChanged);
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.roomStore);
    },

    _onActiveRoomStateChanged: function() {
      
      
      
      if (this.isMounted()) {
        this.setState(this.props.roomStore.getStoreState("activeRoom"));
      }
    },

    getInitialState: function() {
      var storeState = this.props.roomStore.getStoreState("activeRoom");
      return _.extend({
        
        roomState: this.props.roomState || storeState.roomState
      }, storeState);
    }
  };

  


  var DesktopRoomInvitationView = React.createClass({displayName: 'DesktopRoomInvitationView',
    mixins: [ActiveRoomStoreMixin, React.addons.LinkedStateMixin],

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired
    },

    getInitialState: function() {
      return {
        copiedUrl: false,
        newRoomName: "",
        error: null,
      };
    },

    componentWillMount: function() {
      this.listenTo(this.props.roomStore, "change:error",
                    this.onRoomError);
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.roomStore);
    },

    handleTextareaKeyDown: function(event) {
      
      
      
      
      if (event.which === 13) {
        this.handleFormSubmit(event);
      }
    },

    handleFormSubmit: function(event) {
      event.preventDefault();

      this.props.dispatcher.dispatch(new sharedActions.RenameRoom({
        roomToken: this.state.roomToken,
        newRoomName: this.state.newRoomName
      }));
    },

    handleEmailButtonClick: function(event) {
      event.preventDefault();

      this.props.dispatcher.dispatch(
        new sharedActions.EmailRoomUrl({roomUrl: this.state.roomUrl}));
    },

    handleCopyButtonClick: function(event) {
      event.preventDefault();

      this.props.dispatcher.dispatch(
        new sharedActions.CopyRoomUrl({roomUrl: this.state.roomUrl}));

      this.setState({copiedUrl: true});
    },

    onRoomError: function() {
      
      
      
      if (this.isMounted()) {
        this.setState({error: this.props.roomStore.getStoreState("error")});
      }
    },

    render: function() {
      var cx = React.addons.classSet;
      return (
        React.DOM.div({className: "room-invitation-overlay"}, 
          React.DOM.p({className: cx({"error": !!this.state.error,
                            "error-display-area": true})}, 
            mozL10n.get("rooms_name_change_failed_label")
          ), 
          React.DOM.form({onSubmit: this.handleFormSubmit}, 
            React.DOM.textarea({rows: "2", type: "text", className: "input-room-name", 
              valueLink: this.linkState("newRoomName"), 
              onBlur: this.handleFormSubmit, 
              onKeyDown: this.handleTextareaKeyDown, 
              placeholder: mozL10n.get("rooms_name_this_room_label")})
          ), 
          React.DOM.p(null, mozL10n.get("invite_header_text")), 
          React.DOM.div({className: "btn-group call-action-group"}, 
            React.DOM.button({className: "btn btn-info btn-email", 
                    onClick: this.handleEmailButtonClick}, 
              mozL10n.get("share_button2")
            ), 
            React.DOM.button({className: "btn btn-info btn-copy", 
                    onClick: this.handleCopyButtonClick}, 
              this.state.copiedUrl ? mozL10n.get("copied_url_button") :
                                      mozL10n.get("copy_url_button2")
            )
          )
        )
      );
    }
  });

  


  var DesktopRoomConversationView = React.createClass({displayName: 'DesktopRoomConversationView',
    mixins: [
      ActiveRoomStoreMixin,
      sharedMixins.DocumentTitleMixin,
      sharedMixins.RoomsAudioMixin
    ],

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
      feedbackStore:
        React.PropTypes.instanceOf(loop.store.FeedbackStore).isRequired,
    },

    _renderInvitationOverlay: function() {
      if (this.state.roomState !== ROOM_STATES.HAS_PARTICIPANTS) {
        return DesktopRoomInvitationView({
          roomStore: this.props.roomStore, 
          dispatcher: this.props.dispatcher}
        );
      }
      return null;
    },

    componentDidMount: function() {
      





      window.addEventListener('orientationchange', this.updateVideoContainer);
      window.addEventListener('resize', this.updateVideoContainer);
    },

    componentWillUpdate: function(nextProps, nextState) {
      
      
      
      if (this.state.roomState !== ROOM_STATES.MEDIA_WAIT &&
          nextState.roomState === ROOM_STATES.MEDIA_WAIT) {
        this.props.dispatcher.dispatch(new sharedActions.SetupStreamElements({
          publisherConfig: this._getPublisherConfig(),
          getLocalElementFunc: this._getElement.bind(this, ".local"),
          getRemoteElementFunc: this._getElement.bind(this, ".remote")
        }));
      }
    },

    _getPublisherConfig: function() {
      
      
      return {
        insertMode: "append",
        width: "100%",
        height: "100%",
        publishVideo: !this.state.videoMuted,
        style: {
          audioLevelDisplayMode: "off",
          bugDisplayMode: "off",
          buttonDisplayMode: "off",
          nameDisplayMode: "off",
          videoDisabledDisplayMode: "off"
        }
      };
    },

    



    updateVideoContainer: function() {
      var localStreamParent = this._getElement('.local .OT_publisher');
      var remoteStreamParent = this._getElement('.remote .OT_subscriber');
      if (localStreamParent) {
        localStreamParent.style.width = "100%";
      }
      if (remoteStreamParent) {
        remoteStreamParent.style.height = "100%";
      }
    },

    




    _getElement: function(className) {
      return this.getDOMNode().querySelector(className);
    },

    


    leaveRoom: function() {
      this.props.dispatcher.dispatch(new sharedActions.LeaveRoom());
    },

    


    closeWindow: function() {
      window.close();
    },

    





    publishStream: function(type, enabled) {
      this.props.dispatcher.dispatch(
        new sharedActions.SetMute({
          type: type,
          enabled: enabled
        }));
    },

    render: function() {
      if (this.state.roomName) {
        this.setTitle(this.state.roomName);
      }

      var localStreamClasses = React.addons.classSet({
        local: true,
        "local-stream": true,
        "local-stream-audio": this.state.videoMuted,
        "room-preview": this.state.roomState !== ROOM_STATES.HAS_PARTICIPANTS
      });

      switch(this.state.roomState) {
        case ROOM_STATES.FAILED:
        case ROOM_STATES.FULL: {
          
          
          return loop.conversation.GenericFailureView({
            cancelCall: this.closeWindow}
          );
        }
        case ROOM_STATES.ENDED: {
          if (this.state.used)
            return sharedViews.FeedbackView({
              feedbackStore: this.props.feedbackStore, 
              onAfterFeedbackReceived: this.closeWindow}
            );

          
          
          this.closeWindow();
          return null;
        }
        default: {
          return (
            React.DOM.div({className: "room-conversation-wrapper"}, 
              this._renderInvitationOverlay(), 
              React.DOM.div({className: "video-layout-wrapper"}, 
                React.DOM.div({className: "conversation room-conversation"}, 
                  React.DOM.div({className: "media nested"}, 
                    React.DOM.div({className: "video_wrapper remote_wrapper"}, 
                      React.DOM.div({className: "video_inner remote"})
                    ), 
                    React.DOM.div({className: localStreamClasses})
                  ), 
                  sharedViews.ConversationToolbar({
                    video: {enabled: !this.state.videoMuted, visible: true}, 
                    audio: {enabled: !this.state.audioMuted, visible: true}, 
                    publishStream: this.publishStream, 
                    hangup: this.leaveRoom})
                )
              )
            )
          );
        }
      }
    }
  });

  return {
    ActiveRoomStoreMixin: ActiveRoomStoreMixin,
    DesktopRoomConversationView: DesktopRoomConversationView,
    DesktopRoomInvitationView: DesktopRoomInvitationView
  };

})(document.mozL10n || navigator.mozL10n);
