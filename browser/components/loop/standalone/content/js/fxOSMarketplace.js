

var loop = loop || {};
loop.fxOSMarketplaceViews = (function() {
  "use strict";

  







  var FxOSHiddenMarketplaceView = React.createClass({displayName: "FxOSHiddenMarketplaceView",
    render: function() {
      return React.createElement("iframe", {id: "marketplace", src: this.props.marketplaceSrc, hidden: true});
    },

    componentDidUpdate: function() {
      
      if (this.props.onMarketplaceMessage) {
        
        
        
        
        
        window.addEventListener("message", this.props.onMarketplaceMessage);
      }
    }
  });

  return {
    FxOSHiddenMarketplaceView: FxOSHiddenMarketplaceView
  };

})();
