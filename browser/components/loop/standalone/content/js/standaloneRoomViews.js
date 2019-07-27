







var loop = loop || {};
loop.standaloneRoomViews = (function() {
  "use strict";

  var StandaloneRoomView = React.createClass({displayName: 'StandaloneRoomView',
    mixins: [Backbone.Events],

    propTypes: {
      activeRoomStore:
        React.PropTypes.instanceOf(loop.store.ActiveRoomStore).isRequired
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

    render: function() {
      return (
        React.DOM.div(null, 
          React.DOM.div(null, this.state.roomState)
        )
      );
    }
  });

  return {
    StandaloneRoomView: StandaloneRoomView
  };
})();
