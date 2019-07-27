







var loop = loop || {};
loop.roomViews = (function(mozL10n) {
  "use strict";

  var DesktopRoomView = React.createClass({displayName: 'DesktopRoomView',
    mixins: [Backbone.Events, loop.shared.mixins.DocumentTitleMixin],

    propTypes: {
      mozLoop:   React.PropTypes.object.isRequired,
      roomStore: React.PropTypes.instanceOf(loop.store.RoomStore).isRequired,
    },

    getInitialState: function() {
      return this.props.roomStore.getStoreState();
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

    render: function() {
      if (this.state.serverData && this.state.serverData.roomName) {
        this.setTitle(this.state.serverData.roomName);
      }

      return (
        React.DOM.div({className: "goat"})
      );
    }
  });

  return {
    DesktopRoomView: DesktopRoomView
  };

})(document.mozL10n || navigator.mozL10n);
