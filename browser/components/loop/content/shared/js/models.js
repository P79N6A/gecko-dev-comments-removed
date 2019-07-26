





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.models = (function() {
  "use strict";

  


  var ConversationModel = Backbone.Model.extend({
    defaults: {
      callerId:     undefined, 
      loopToken:    undefined, 
      loopVersion:  undefined, 
                               
                               
                               
      sessionId:    undefined, 
      sessionToken: undefined, 
      apiKey:       undefined  
    },

    


















    initiate: function(options) {
      
      if (this.isSessionReady()) {
        return this.trigger("session:ready", this);
      }

      var client = new loop.shared.Client({
        baseServerUrl: options.baseServerUrl
      });

      function handleResult(err, sessionData) {
        
        if (err) {
          this.trigger("session:error", new Error(
            "Retrieval of session information failed: HTTP " + err));
          return;
        }

        
        
        
        
        
        if (!options.outgoing)
          sessionData = sessionData[0];

        this.setReady(sessionData);
      }

      if (options.outgoing) {
        client.requestCallInfo(this.get("loopToken"), handleResult.bind(this));
      }
      else {
        client.requestCallsInfo(this.get("loopVersion"),
          handleResult.bind(this));
      }
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

  


  var NotificationModel = Backbone.Model.extend({
    defaults: {
      level: "info",
      message: ""
    }
  });

  


  var NotificationCollection = Backbone.Collection.extend({
    model: NotificationModel
  });

  return {
    ConversationModel: ConversationModel,
    NotificationCollection: NotificationCollection,
    NotificationModel: NotificationModel
  };
})();
