







var loop = loop || {};
loop.roomViews = (function(mozL10n) {
  "use strict";

  var DesktopRoomView = React.createClass({displayName: 'DesktopRoomView',
    mixins: [Backbone.Events, loop.shared.mixins.DocumentTitleMixin],

    propTypes: {
      mozLoop:
        React.PropTypes.object.isRequired,
      localRoomStore:
        React.PropTypes.instanceOf(loop.store.LocalRoomStore).isRequired,
    },

    getInitialState: function() {
      return this.props.localRoomStore.getStoreState();
    },

    componentWillMount: function() {
      this.listenTo(this.props.localRoomStore, "change",
        this._onLocalRoomStoreChanged);
    },

    





    _onLocalRoomStoreChanged: function() {
      this.setState(this.props.localRoomStore.getStoreState());
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.localRoomStore);
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

})(document.mozL10n || navigator.mozL10n);;
