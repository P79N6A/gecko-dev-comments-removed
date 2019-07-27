





var loop = loop || {};
loop.FeedbackAPIClient = (function($, _) {
  "use strict";

  

















  function FeedbackAPIClient(baseUrl, defaults) {
    this.baseUrl = baseUrl;
    if (!this.baseUrl) {
      throw new Error("Missing required 'baseUrl' argument.");
    }

    this.defaults = defaults || {};
    
    if (!this.defaults.hasOwnProperty("product")) {
      throw new Error("Missing required 'product' default.");
    }
  }

  FeedbackAPIClient.prototype = {
    



    _supportedFields: ["happy",
                       "category",
                       "description",
                       "product",
                       "platform",
                       "version",
                       "channel",
                       "user_agent",
                       "url"],

    







    _createPayload: function(fields) {
      if (typeof fields !== "object") {
        throw new Error("Invalid feedback data provided.");
      }

      Object.keys(fields).forEach(function(name) {
        if (this._supportedFields.indexOf(name) === -1) {
          throw new Error("Unsupported field " + name);
        }
      }, this);

      
      var payload = _.extend({}, this.defaults, fields);

      
      if (!fields.description) {
        payload.description = (fields.happy ? "Happy" : "Sad") + " User";
      }

      return payload;
    },

    





    send: function(fields, cb) {
      var req = $.ajax({
        url:         this.baseUrl,
        method:      "POST",
        contentType: "application/json",
        dataType:    "json",
        data: JSON.stringify(this._createPayload(fields))
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
})(jQuery, _);
