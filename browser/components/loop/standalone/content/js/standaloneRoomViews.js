







var loop = loop || {};
loop.standaloneRoomViews = (function() {
  "use strict";

  var StandaloneRoomView = React.createClass({displayName: 'StandaloneRoomView',
    render: function() {
      return (React.DOM.div(null, "Room"));
    }
  });

  return {
    StandaloneRoomView: StandaloneRoomView
  };
})();
