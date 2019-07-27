






var loop = loop || {};
loop.Client = (function($) {
  "use strict";

  
  var expectedCallUrlProperties = ["callUrl", "expiresAt"];

  
  var expectedCallProperties = ["calls"];

  




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
      cb(new Error(message));
    },

    







    _ensureRegistered: function(cb) {
      this.mozLoop.ensureRegistered(function(error) {
        if (error) {
          console.log("Error registering with Loop server, code: " + error);
          cb(error);
          return;
        } else {
          cb(null);
        }
      });
    },

    











    _requestCallUrlInternal: function(nickname, cb) {
      this.mozLoop.hawkRequest("/call-url/", "POST", {callerId: nickname},
                               function (error, responseText) {
        if (error) {
          this._telemetryAdd("LOOP_CLIENT_CALL_URL_REQUESTS_SUCCESS", false);
          this._failureHandler(cb, error);
          return;
        }

        try {
          var urlData = JSON.parse(responseText);

          
          
          var returnData = this._validate(urlData, expectedCallUrlProperties);

          this._telemetryAdd("LOOP_CLIENT_CALL_URL_REQUESTS_SUCCESS", true);
          cb(null, returnData);
        } catch (err) {
          this._telemetryAdd("LOOP_CLIENT_CALL_URL_REQUESTS_SUCCESS", false);
          console.log("Error requesting call info", err);
          cb(err);
        }
      }.bind(this));
    },

    








    deleteCallUrl: function(token, cb) {
      this._ensureRegistered(function(err) {
        if (err) {
          cb(err);
          return;
        }

        this._deleteCallUrlInternal(token, cb);
      }.bind(this));
    },

    _deleteCallUrlInternal: function(token, cb) {
      this.mozLoop.hawkRequest("/call-url/" + token, "DELETE", null,
                               function (error, responseText) {
        if (error) {
          this._failureHandler(cb, error);
          return;
        }

        try {
          cb(null);
        } catch (err) {
          console.log("Error deleting call info", err);
          cb(err);
        }
      }.bind(this));
    },

    













    requestCallUrl: function(nickname, cb) {
      this._ensureRegistered(function(err) {
        if (err) {
          cb(err);
          return;
        }

        this._requestCallUrlInternal(nickname, cb);
      }.bind(this));
    },

    





    _telemetryAdd: function(histogramId, value) {
      try {
        this.mozLoop.telemetryAdd(histogramId, value);
      } catch (err) {
        console.error("Error recording telemetry", err);
      }
    },
  };

  return Client;
})(jQuery);
