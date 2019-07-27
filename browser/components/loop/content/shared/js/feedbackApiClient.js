





var loop = loop || {};
loop.FeedbackAPIClient = (function($) {
  "use strict";

  










  function FeedbackAPIClient(settings) {
    settings = settings || {};
    if (!settings.hasOwnProperty("baseUrl")) {
      throw new Error("Missing required baseUrl setting.");
    }
    this._baseUrl = settings.baseUrl;
    if (!settings.hasOwnProperty("product")) {
      throw new Error("Missing required product setting.");
    }
    this._product = settings.product;
  }

  FeedbackAPIClient.prototype = {
    





    _formatData: function(fields) {
      var formatted = {};

      if (typeof fields !== "object") {
        throw new Error("Invalid feedback data provided.");
      }

      formatted.product = this._product;
      formatted.happy = fields.happy;
      formatted.category = fields.category;

      
      if (!fields.description) {
        formatted.description = (fields.happy ? "Happy" : "Sad") + " User";
      } else {
        formatted.description = fields.description;
      }

      return formatted;
    },

    





    send: function(fields, cb) {
      var req = $.ajax({
        url:         this._baseUrl,
        method:      "POST",
        contentType: "application/json",
        dataType:    "json",
        data: JSON.stringify(this._formatData(fields))
      });

      req.done(function(result) {
        console.info("User feedback data have been submitted", result);
        cb(null, result);
      });

      req.fail(function(jqXHR, textStatus, errorThrown) {
        var message = "Error posting user feedback data";
        var httpError = jqXHR.status + " " + errorThrown;
        console.error(message, httpError, JSON.stringify(jqXHR.responseJSON));
        cb(new Error(message + ": " + httpError));
      });
    }
  };

  return FeedbackAPIClient;
})(jQuery);
