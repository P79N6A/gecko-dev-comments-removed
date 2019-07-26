






var loop = loop || {};
loop.conversation = (function(_, __) {
  "use strict";

  
  var baseApiUrl = "http://localhost:5000";

  


  function init() {
    
    this.client = new loop.Client({
      baseApiUrl: baseApiUrl
    });

    
    this.client.requestCallsInfo(function(err, calls) {
      if (err) {
        console.error("Error getting call data: ", err);
        return;
      }

      console.log("Received Calls Data: ", calls);
    });
  }

  return {
    init: init
  };
})(_);
