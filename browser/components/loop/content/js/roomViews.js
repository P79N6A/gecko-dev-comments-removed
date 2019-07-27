







var loop = loop || {};
loop.roomViews = (function(mozL10n) {
  "use strict";

  var ROOM_STATES = loop.store.ROOM_STATES;

  var DesktopRoomView = React.createClass({displayName: 'DesktopRoomView',
    mixins: [Backbone.Events, loop.shared.mixins.DocumentTitleMixin],

    propTypes: {
      mozLoop:   React.PropTypes.object.isRequired,
      roomStore: React.PropTypes.instanceOf(loop.store.RoomStore).isRequired,
    },

    getInitialState: function() {
      return this.props.roomStore.getStoreState("activeRoom");
    },

    componentWillMount: function() {
      this.listenTo(this.props.roomStore, "change:activeRoom",
                    this._onActiveRoomStateChanged);
    },

    





    _onActiveRoomStateChanged: function() {
      this.setState(this.props.roomStore.getStoreState("activeRoom"));
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.roomStore);
    },

    


    closeWindow: function() {
      window.close();
    },

    render: function() {
      if (this.state.roomName) {
        this.setTitle(this.state.roomName);
      }

      if (this.state.roomState === ROOM_STATES.FAILED) {
        return (loop.conversation.GenericFailureView({
          cancelCall: this.closeWindow}
        ));
      }

      return (
        React.DOM.div(null, 
          React.DOM.div(null, mozL10n.get("invite_header_text"))
        )
      );
    }
  });

  return {
    DesktopRoomView: DesktopRoomView
  };

})(document.mozL10n || navigator.mozL10n);
