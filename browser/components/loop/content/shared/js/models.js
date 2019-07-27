





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.models = (function() {
  "use strict";

  


  var ConversationModel = Backbone.Model.extend({
    defaults: {
      ongoing:      false,     
      callerId:     undefined, 
      loopToken:    undefined, 
      loopVersion:  undefined, 
                               
                               
                               
      sessionId:    undefined, 
      sessionToken: undefined, 
      apiKey:       undefined  
    },

    



    sdk: undefined,

    



    session: undefined,

    








    initialize: function(attributes, options) {
      options = options || {};
      if (!options.sdk) {
        throw new Error("missing required sdk");
      }
      this.sdk = options.sdk;
    },

    






















    initiate: function(options) {
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
        options.client.requestCallInfo(this.get("loopToken"), options.callType,
          handleResult.bind(this));
      }
      else {
        options.client.requestCallsInfo(this.get("loopVersion"),
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
    },

    


    startSession: function() {
      if (!this.isSessionReady()) {
        throw new Error("Can't start session as it's not ready");
      }
      this.session = this.sdk.initSession(this.get("sessionId"));
      this.listenTo(this.session, "streamCreated", this._streamCreated);
      this.listenTo(this.session, "connectionDestroyed",
                                  this._connectionDestroyed);
      this.listenTo(this.session, "sessionDisconnected",
                                  this._sessionDisconnected);
      this.listenTo(this.session, "networkDisconnected",
                                  this._networkDisconnected);
      this.session.connect(this.get("apiKey"), this.get("sessionToken"),
                           this._onConnectCompletion.bind(this));
    },

    


    endSession: function() {
      this.session.disconnect();
      this.once("session:ended", this.stopListening, this);
      this.set("ongoing", false);
    },

    








    _onConnectCompletion: function(error) {
      if (error) {
        this.trigger("session:connection-error", error);
        this.endSession();
      } else {
        this.trigger("session:connected");
        this.set("ongoing", true);
      }
    },

    





    _streamCreated: function(event) {
      this.trigger("session:stream-created", event);
    },

    





    _sessionDisconnected: function(event) {
      this.trigger("session:ended");
      this.set("ongoing", false);
    },

    





    _connectionDestroyed: function(event) {
      this.trigger("session:peer-hungup", {
        connectionId: event.connection.connectionId
      });
      this.endSession();
    },

    





    _networkDisconnected: function(event) {
      this.trigger("session:network-disconnected");
      this.endSession();
    },
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
