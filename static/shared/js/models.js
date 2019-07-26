





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.models = (function() {
  "use strict";

  


  var ConversationModel = Backbone.Model.extend({
    defaults: {
      loopToken:    undefined, 
      sessionId:    undefined, 
      sessionToken: undefined, 
      apiKey:       undefined  
    },

    














    initiate: function(baseServerUrl) {

      if (!baseServerUrl) {
        throw new Error("baseServerUrl arg must be passed to initiate()");
      }

      if (!this.get("loopToken")) {
        throw new Error("missing required attribute loopToken");
      }

      
      if (this.isSessionReady()) {
        return this.trigger("session:ready", this);
      }

      var request = $.ajax({
        url:         baseServerUrl + "/calls/" + this.get("loopToken"),
        method:      "POST",
        contentType: "application/json",
        data:        JSON.stringify({}),
        dataType:    "json"
      });

      request.done(this.setReady.bind(this));

      request.fail(function(xhr, _, statusText) {
        var serverError = xhr.status + " " + statusText;
        if (typeof xhr.responseJSON === "object" && xhr.responseJSON.error) {
          serverError += "; " + xhr.responseJSON.error;
        }
        this.trigger("session:error", new Error(
          "Retrieval of session information failed: HTTP " + serverError));
      }.bind(this));
    },

    




    isSessionReady: function() {
      return !!this.get("sessionId");
    },

    




    setReady: function(sessionData) {
      
      this.set({
        sessionId:    sessionData.sessionId,
        sessionToken: sessionData.sessionToken,
        apiKey:       sessionData.apiKey
      }).trigger("session:ready", this);
      return this;
    }
  });

  return {
    ConversationModel: ConversationModel
  };
})();
