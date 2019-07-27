





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.models = (function() {
  "use strict";

  


  var ConversationModel = Backbone.Model.extend({
    defaults: {
      connected:    false,     
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

    



    pendingCallTimeout: undefined,

    



    _pendingCallTimer: undefined,

    














    initialize: function(attributes, options) {
      options = options || {};
      if (!options.sdk) {
        throw new Error("missing required sdk");
      }
      this.sdk = options.sdk;
      this.pendingCallTimeout = options.pendingCallTimeout || 20000;

      
      this.on("session:ended session:error", this._clearPendingCallTimer, this);
    },

    






















    initiate: function(options) {
      options = options || {};

      
      function handleOutgoingCallTimeout() {
        
        if (!this.get("ongoing")) {
          this.trigger("timeout").endSession();
        }
      }

      function handleResult(err, sessionData) {
        
        this._clearPendingCallTimer();

        if (err) {
          this.trigger("session:error", new Error(
            "Retrieval of session information failed: HTTP " + err));
          return;
        }

        if (options.outgoing) {
          
          this._pendingCallTimer = setTimeout(
            handleOutgoingCallTimeout.bind(this), this.pendingCallTimeout);
        } else {
          
          
          
          
          
          sessionData = sessionData[0];
        }

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
      this.set("ongoing", false)
          .once("session:ended", this.stopListening, this);
    },

    


    _clearPendingCallTimer: function() {
      if (this._pendingCallTimer) {
        clearTimeout(this._pendingCallTimer);
      }
    },

    








    _onConnectCompletion: function(error) {
      if (error) {
        this.trigger("session:connection-error", error);
        this.endSession();
      } else {
        this.trigger("session:connected");
        this.set("connected", true);
      }
    },

    





    _streamCreated: function(event) {
      this.set("ongoing", true)
          .trigger("session:stream-created", event);
    },

    





    _sessionDisconnected: function(event) {
      this.set("connected", false)
          .set("ongoing", false)
          .trigger("session:ended");
    },

    





    _connectionDestroyed: function(event) {
      this.set("connected", false)
          .set("ongoing", false)
          .trigger("session:peer-hungup", {
            connectionId: event.connection.connectionId
          });
      this.endSession();
    },

    





    _networkDisconnected: function(event) {
      this.set("connected", false)
          .set("ongoing", false)
          .trigger("session:network-disconnected");
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
