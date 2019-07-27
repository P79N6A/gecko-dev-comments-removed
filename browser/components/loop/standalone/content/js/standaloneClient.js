





var loop = loop || {};
loop.StandaloneClient = (function($) {
  "use strict";

  
  var expectedCallsProperties = [ "sessionId", "sessionToken", "apiKey" ];

  




  function StandaloneClient(settings) {
    settings = settings || {};
    if (!settings.baseServerUrl) {
      throw new Error("missing required baseServerUrl");
    }

    this.settings = settings;
  }

  StandaloneClient.prototype = {
    







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

      if (properties.length === 1) {
        return data[properties[0]];
      }

      return data;
    },

    







    _failureHandler: function(cb, jqXHR, textStatus, errorThrown) {
      var jsonErr = jqXHR && jqXHR.responseJSON || {};
      var message = "HTTP " + jqXHR.status + " " + errorThrown;

      
      console.error("Server error", message, jsonErr);

      
      var err = new Error(message);
      err.errno = jsonErr.errno;

      cb(err);
    },

   






    requestCallUrlInfo: function(loopToken, cb) {
      if (!loopToken) {
        throw new Error("Missing required parameter loopToken");
      }
      if (!cb) {
        throw new Error("Missing required callback function");
      }

      $.get(this.settings.baseServerUrl + "/calls/" + loopToken)
        .done(function(callUrlInfo) {
          cb(null, callUrlInfo);
        }).fail(this._failureHandler.bind(this, cb));
    },

    








    requestCallInfo: function(loopToken, callType, cb) {
      if (!loopToken) {
        throw new Error("missing required parameter loopToken");
      }

      var req = $.ajax({
        url:         this.settings.baseServerUrl + "/calls/" + loopToken,
        method:      "POST",
        contentType: "application/json",
        dataType:    "json",
        data: JSON.stringify({callType: callType})
      });

      req.done(function(sessionData) {
        try {
          cb(null, this._validate(sessionData, expectedCallsProperties));
        } catch (err) {
          console.error("Error requesting call info", err.message);
          cb(err);
        }
      }.bind(this));

      req.fail(this._failureHandler.bind(this, cb));
    },
  };

  return StandaloneClient;
})(jQuery);
