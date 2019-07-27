





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

      if (properties.length == 1) {
        return data[properties[0]];
      }

      return data;
    },

    







    _failureHandler: function(cb, jqXHR, textStatus, errorThrown) {
      var error = "Unknown error.",
          jsonRes = jqXHR && jqXHR.responseJSON || {};
      
      
      
      
      
      
      
      if (jsonRes.status === "errors" && Array.isArray(jsonRes.errors)) {
        error = "Details: " + jsonRes.errors.map(function(err) {
          return Object.keys(err).map(function(field) {
            return field + ": " + err[field];
          }).join(", ");
        }).join("; ");
      }
      var message = "HTTP " + jqXHR.status + " " + errorThrown +
          "; " + error;
      console.error(message);
      cb(new Error(message));
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
          console.log("Error requesting call info", err);
          cb(err);
        }
      }.bind(this));

      req.fail(this._failureHandler.bind(this, cb));
    },
  };

  return StandaloneClient;
})(jQuery);
