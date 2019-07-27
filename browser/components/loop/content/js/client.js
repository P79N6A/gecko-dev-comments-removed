



var loop = loop || {};
loop.Client = (function($) {
  "use strict";

  
  var expectedPostCallProperties = [
    "apiKey", "callId", "progressURL",
    "sessionId", "sessionToken", "websocketToken"
  ];

  




  function Client(settings) {
    if (!settings) {
      settings = {};
    }
    
    
    if ("mozLoop" in settings) {
      this.mozLoop = settings.mozLoop;
    } else {
      this.mozLoop = navigator.mozLoop;
    }

    this.settings = settings;
  }

  Client.prototype = {
    







    _validate: function(data, properties) {
      if (typeof data !== "object") {
        throw new Error("Invalid data received from server");
      }

      properties.forEach(function (property) {
        if (!data.hasOwnProperty(property)) {
          throw new Error("Invalid data received from server - missing " +
            property);
        }
      });

      if (properties.length == 1) {
        return data[properties[0]];
      }

      return data;
    },

    





    _failureHandler: function(cb, error) {
      var message = "HTTP " + error.code + " " + error.error + "; " + error.message;
      console.error(message);
      cb(error);
    },

    










    setupOutgoingCall: function(calleeIds, callType, cb) {
      
      
      this.mozLoop.hawkRequest(this.mozLoop.LOOP_SESSION_TYPE.FXA,
        "/calls", "POST", {
          calleeId: calleeIds,
          callType: callType,
          channel: this.mozLoop.appVersionInfo ?
                   this.mozLoop.appVersionInfo.channel : "unknown"
        },
        function (err, responseText) {
          if (err) {
            this._failureHandler(cb, err);
            return;
          }

          try {
            var postData = JSON.parse(responseText);

            var outgoingCallData = this._validate(postData,
              expectedPostCallProperties);

            cb(null, outgoingCallData);
          } catch (ex) {
            console.log("Error requesting call info", ex);
            cb(ex);
          }
        }.bind(this)
      );
    }
  };

  return Client;
})(jQuery);
