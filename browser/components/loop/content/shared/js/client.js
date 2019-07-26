





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.Client = (function($) {
  "use strict";

  




  function Client(settings) {
    settings = settings || {};
    if (!settings.hasOwnProperty("baseServerUrl") ||
        !settings.baseServerUrl) {
      throw new Error("missing required baseServerUrl");
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

      if (properties.length <= 1) {
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

    







    _ensureRegistered: function(cb) {
      navigator.mozLoop.ensureRegistered(function(err) {
        cb(err);
      }.bind(this));
    },

    









    _requestCallUrlInternal: function(nickname, cb) {
      var endpoint = this.settings.baseServerUrl + "/call-url/",
          reqData  = {callerId: nickname};

      var req = $.post(endpoint, reqData, function(callUrlData) {
        try {
          cb(null, this._validate(callUrlData, ["call_url"]));
        } catch (err) {
          console.log("Error requesting call info", err);
          cb(err);
        }
      }.bind(this), "json");

      req.fail(this._failureHandler.bind(this, cb));
    },

    










    requestCallUrl: function(nickname, cb) {
      this._ensureRegistered(function(err) {
        if (err) {
          console.log("Error registering with Loop server, code: " + err);
          cb(err);
          return;
        }

        this._requestCallUrlInternal(nickname, cb);
      }.bind(this));
    },

    







    requestCallsInfo: function(version, cb) {
      if (!version) {
        throw new Error("missing required parameter version");
      }

      var endpoint = this.settings.baseServerUrl + "/calls";

      
      
      
      
      var req = $.get(endpoint + "?version=" + version, function(callsData) {
        try {
          cb(null, this._validate(callsData, ["calls"]));
        } catch (err) {
          console.log("Error requesting calls info", err);
          cb(err);
        }
      }.bind(this), "json");

      req.fail(this._failureHandler.bind(this, cb));
    },

    






    requestCallInfo: function(loopToken, cb) {
      if (!loopToken) {
        throw new Error("missing required parameter loopToken");
      }

      var req = $.ajax({
        url:         this.settings.baseServerUrl + "/calls/" + loopToken,
        method:      "POST",
        contentType: "application/json",
        data:        JSON.stringify({}),
        dataType:    "json"
      });

      req.done(function(sessionData) {
        try {
          cb(null, this._validate(sessionData, [
            "sessionId", "sessionToken", "apiKey"
          ]));
        } catch (err) {
          console.log("Error requesting call info", err);
          cb(err);
        }
      }.bind(this));

      req.fail(this._failureHandler.bind(this, cb));
    }
  };

  return Client;
})(jQuery);
