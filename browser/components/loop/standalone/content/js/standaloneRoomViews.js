







var loop = loop || {};
loop.standaloneRoomViews = (function(mozL10n) {
  "use strict";

  var ROOM_STATES = loop.store.ROOM_STATES;
  var sharedActions = loop.shared.actions;
  var sharedViews = loop.shared.views;

  var StandaloneRoomView = React.createClass({displayName: 'StandaloneRoomView',
    mixins: [Backbone.Events],

    propTypes: {
      activeRoomStore:
        React.PropTypes.instanceOf(loop.store.ActiveRoomStore).isRequired,
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
    },

    getInitialState: function() {
      var storeState = this.props.activeRoomStore.getStoreState();
      return _.extend({}, storeState, {
        
        roomState: this.props.roomState || storeState.roomState
      });
    },

    componentWillMount: function() {
      this.listenTo(this.props.activeRoomStore, "change",
                    this._onActiveRoomStateChanged);
    },

    





    _onActiveRoomStateChanged: function() {
      this.setState(this.props.activeRoomStore.getStoreState());
    },

    




    _getElement: function(className) {
      return this.getDOMNode().querySelector(className);
    },

     


    _getPublisherConfig: function() {
      
      
      return {
        insertMode: "append",
        width: "100%",
        height: "100%",
        publishVideo: true,
        style: {
          audioLevelDisplayMode: "off",
          bugDisplayMode: "off",
          buttonDisplayMode: "off",
          nameDisplayMode: "off",
          videoDisabledDisplayMode: "off"
        }
      };
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.activeRoomStore);
    },

    





    componentWillUpdate: function(nextProps, nextState) {
      if (this.state.roomState === ROOM_STATES.READY &&
          nextState.roomState === ROOM_STATES.JOINED) {
        this.props.dispatcher.dispatch(new sharedActions.SetupStreamElements({
          publisherConfig: this._getPublisherConfig(),
          getLocalElementFunc: this._getElement.bind(this, ".local"),
          getRemoteElementFunc: this._getElement.bind(this, ".remote")
        }));
      }
    },

    joinRoom: function() {
      this.props.dispatcher.dispatch(new sharedActions.JoinRoom());
    },

    leaveRoom: function() {
      this.props.dispatcher.dispatch(new sharedActions.LeaveRoom());
    },

    





    publishStream: function(type, enabled) {
      this.props.dispatcher.dispatch(new sharedActions.SetMute({
        type: type,
        enabled: enabled
      }));
    },

    




    _roomIsActive: function() {
      return this.state.roomState === ROOM_STATES.JOINED            ||
             this.state.roomState === ROOM_STATES.SESSION_CONNECTED ||
             this.state.roomState === ROOM_STATES.HAS_PARTICIPANTS;
    },

    _renderActionButtons: function() {
      
      
      if (this.state.roomState === ROOM_STATES.INIT ||
          this.state.roomState === ROOM_STATES.READY) {
        return (
          React.DOM.div({className: "room-inner-action-area"}, 
            React.DOM.button({className: "btn btn-join btn-info", onClick: this.joinRoom}, 
              mozL10n.get("rooms_room_join_label")
            )
          )
        );
      }
    },

    render: function() {
      var localStreamClasses = React.addons.classSet({
        hide: !this._roomIsActive(),
        local: true,
        "local-stream": true,
        "local-stream-audio": false
      });

      return (
        React.DOM.div({className: "room-conversation-wrapper"}, 
          this._renderActionButtons(), 
          React.DOM.div({className: "video-layout-wrapper"}, 
            React.DOM.div({className: "conversation room-conversation"}, 
              React.DOM.h2({className: "room-name"}, this.state.roomName), 
              React.DOM.div({className: "media nested"}, 
                React.DOM.div({className: "video_wrapper remote_wrapper"}, 
                  React.DOM.div({className: "video_inner remote"})
                ), 
                React.DOM.div({className: localStreamClasses})
              ), 
              sharedViews.ConversationToolbar({
                video: {enabled: !this.state.videoMuted,
                        visible: this._roomIsActive()}, 
                audio: {enabled: !this.state.audioMuted,
                        visible: this._roomIsActive()}, 
                publishStream: this.publishStream, 
                hangup: this.leaveRoom, 
                hangupButtonLabel: mozL10n.get("rooms_leave_button_label"), 
                enableHangup: this._roomIsActive()})
            )
          )
        )
      );
    }
  });

  return {
    StandaloneRoomView: StandaloneRoomView
  };
})(navigator.mozL10n);
