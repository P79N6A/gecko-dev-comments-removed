





var loop = loop || {};
loop.Client = (function($) {
  "use strict";

  




  function Client(settings) {
    settings = settings || {};
    if (!settings.hasOwnProperty("baseApiUrl")) {
      throw new Error("missing required baseApiUrl");
    }
    this.settings = settings;
  }

  Client.prototype = {
    




    requestCallUrl: function(simplepushUrl, cb) {
      var endpoint = this.settings.baseApiUrl + "/call-url/",
          reqData = {simplepushUrl: simplepushUrl};

      function validate(callUrlData) {
        if (typeof callUrlData !== "object" ||
            !callUrlData.hasOwnProperty("call_url")) {
          var message = "Invalid call url data received";
          console.error(message, callUrlData);
          throw new Error(message);
        }
        return callUrlData.call_url;
      }

      var req = $.post(endpoint, reqData, function(callUrlData) {
        try {
          cb(null, validate(callUrlData));
        } catch (err) {
          cb(err);
        }
      }, "json");

      req.fail(function(jqXHR, testStatus, errorThrown) {
        var error = "Unknown error.";
        if (jqXHR && jqXHR.responseJSON && jqXHR.responseJSON.error) {
          error = jqXHR.responseJSON.error;
        }
        var message = "HTTP error " + jqXHR.status + ": " +
          errorThrown + "; " + error;
        console.error(message);
        cb(new Error(message));
      });
    },

    




    requestCallsInfo: function(cb) {
      var endpoint = this.settings.baseApiUrl + "/calls";

      
      
      function validate(callsData) {
        if (typeof callsData !== "object" ||
            !callsData.hasOwnProperty("calls")) {
          var message = "Invalid calls data received";
          console.error(message, callsData);
          throw new Error(message);
        }
        return callsData.calls;
      }

      
      
      
      
      
      var req = $.get(endpoint + "?version=0", function(callsData) {
        try {
          cb(null, validate(callsData));
        } catch (err) {
          cb(err);
        }
      }, "json");

      req.fail(function(jqXHR, testStatus, errorThrown) {
        var error = "Unknown error.";
        if (jqXHR && jqXHR.responseJSON && jqXHR.responseJSON.error) {
          error = jqXHR.responseJSON.error;
        }
        var message = "HTTP error " + jqXHR.status + ": " +
          errorThrown + "; " + error;
        console.error(message);
        cb(new Error(message));
      });
    }
  };

  return Client;
})(jQuery);
