








var loop = loop || {};
loop.roomViews = (function(mozL10n) {
  "use strict";

  var sharedActions = loop.shared.actions;
  var ROOM_STATES = loop.store.ROOM_STATES;
  var sharedViews = loop.shared.views;

  function noop() {}

  



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
    mixins: [ActiveRoomStoreMixin],

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired
    },

    getInitialState: function() {
      return {
        copiedUrl: false
      }
    },

    handleFormSubmit: function(event) {
      event.preventDefault();
      
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

    render: function() {
      return (
        React.DOM.div({className: "room-invitation-overlay"}, 
          React.DOM.form({onSubmit: this.handleFormSubmit}, 
            React.DOM.input({type: "text", ref: "roomName", 
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
    mixins: [ActiveRoomStoreMixin, loop.shared.mixins.DocumentTitleMixin],

    propTypes: {
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired
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

      
      
      
      this.props.dispatcher.dispatch(new sharedActions.SetupStreamElements({
        publisherConfig: this._getPublisherConfig(),
        getLocalElementFunc: this._getElement.bind(this, ".local"),
        getRemoteElementFunc: this._getElement.bind(this, ".remote")
      }));
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
        "local-stream-audio": !this.state.videoMuted
      });

      switch(this.state.roomState) {
        case ROOM_STATES.FAILED: {
          return loop.conversation.GenericFailureView({
            cancelCall: this.closeWindow}
          );
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
                    hangup: noop})
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
