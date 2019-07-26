





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

      req.fail(function(xhr, type, err) {
        var error = "Unknown error.";
        if (xhr && xhr.responseJSON && xhr.responseJSON.error) {
          error = xhr.responseJSON.error;
        }
        var message = "HTTP error " + xhr.status + ": " + err + "; " + error;
        console.error(message);
        cb(new Error(message));
      });
    }
  };

  return Client;
})(jQuery);
