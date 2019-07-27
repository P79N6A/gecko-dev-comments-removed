







var loop = loop || {};
loop.standaloneRoomViews = (function() {
  "use strict";

  var ROOM_STATES = loop.store.ROOM_STATES;
  var sharedActions = loop.shared.actions;

  var StandaloneRoomView = React.createClass({displayName: 'StandaloneRoomView',
    mixins: [Backbone.Events],

    propTypes: {
      activeRoomStore:
        React.PropTypes.instanceOf(loop.store.ActiveRoomStore).isRequired,
      dispatcher: React.PropTypes.instanceOf(loop.Dispatcher).isRequired,
    },

    getInitialState: function() {
      return this.props.activeRoomStore.getStoreState();
    },

    componentWillMount: function() {
      this.listenTo(this.props.activeRoomStore, "change",
                    this._onActiveRoomStateChanged);
    },

    





    _onActiveRoomStateChanged: function() {
      this.setState(this.props.activeRoomStore.getStoreState());
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.activeRoomStore);
    },

    joinRoom: function() {
      this.props.dispatcher.dispatch(new sharedActions.JoinRoom());
    },

    leaveRoom: function() {
      this.props.dispatcher.dispatch(new sharedActions.LeaveRoom());
    },

    
    
    render: function() {
      switch(this.state.roomState) {
        case ROOM_STATES.READY: {
          return (
            React.DOM.div(null, React.DOM.button({onClick: this.joinRoom}, "Join"))
          );
        }
        case ROOM_STATES.JOINED: {
          return (
            React.DOM.div(null, React.DOM.button({onClick: this.leaveRoom}, "Leave"))
          );
        }
        default: {
          return (
            React.DOM.div(null, this.state.roomState)
          );
        }
      }
    }
  });

  return {
    StandaloneRoomView: StandaloneRoomView
  };
})();
