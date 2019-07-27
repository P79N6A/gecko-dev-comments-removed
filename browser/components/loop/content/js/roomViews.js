







var loop = loop || {};
loop.roomViews = (function(mozL10n) {
  "use strict";

  



  var rootObject = window;

  





  function setRootObject(obj) {
    rootObject = obj;
  }

  var EmptyRoomView = React.createClass({displayName: 'EmptyRoomView',
    mixins: [Backbone.Events],

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

    componentDidMount: function() {
      
      
      if (this.props.mozLoop.rooms && this.props.mozLoop.rooms.addCallback) {
        this.props.mozLoop.rooms.addCallback(
          this.state.localRoomId,
          "RoomCreationError", this.onCreationError);
      }
    },

    






    onCreationError: function(err) {
      
      rootObject.console.error("EmptyRoomView creation error: ", err);
    },

    





    _onLocalRoomStoreChanged: function() {
      this.setState(this.props.localRoomStore.getStoreState());
    },

    componentWillUnmount: function() {
      this.stopListening(this.props.localRoomStore);

      
      
      if (this.props.mozLoop.rooms && this.props.mozLoop.rooms.removeCallback) {
        this.props.mozLoop.rooms.removeCallback(
          this.state.localRoomId,
          "RoomCreationError", this.onCreationError);
      }
    },

    render: function() {
      
      if (this.state.serverData && this.state.serverData.roomName) {
        rootObject.document.title = this.state.serverData.roomName;
      }

      return (
        React.DOM.div({className: "goat"})
      );
    }
  });

  return {
    setRootObject: setRootObject,
    EmptyRoomView: EmptyRoomView
  };

})(document.mozL10n || navigator.mozL10n);;
