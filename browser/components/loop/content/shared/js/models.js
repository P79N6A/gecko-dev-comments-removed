





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.models = (function(l10n) {
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
      apiKey:       undefined,     
      callId:       undefined,     
      progressURL:  undefined,     
      websocketToken: undefined,   
                                   
                                   
      callType:     undefined,     
                                   
      selectedCallType: undefined, 
                                   
      callToken:    undefined,     
                                   
      subscribedStream: false,     
                                   
      publishedStream: false       
                                   
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

    


    incoming: function() {
      this.trigger("call:incoming");
    },

    



    setupOutgoingCall: function() {
      this.trigger("call:outgoing:setup");
    },

    





    outgoing: function(sessionData) {
      this._clearPendingCallTimer();

      
      function handleOutgoingCallTimeout() {
        
        if (!this.get("ongoing")) {
          this.trigger("timeout").endSession();
        }
      }

      
      this._pendingCallTimer = setTimeout(
        handleOutgoingCallTimeout.bind(this), this.pendingCallTimeout);

      this.setOutgoingSessionData(sessionData);
      this.trigger("call:outgoing");
    },

    




    isSessionReady: function() {
      return !!this.get("sessionId");
    },

    





    setOutgoingSessionData: function(sessionData) {
      
      this.set({
        sessionId:      sessionData.sessionId,
        sessionToken:   sessionData.sessionToken,
        apiKey:         sessionData.apiKey,
        callId:         sessionData.callId,
        progressURL:    sessionData.progressURL,
        websocketToken: sessionData.websocketToken.toString(16)
      });
    },

    




    setIncomingSessionData: function(sessionData) {
      
      this.set({
        sessionId:      sessionData.sessionId,
        sessionToken:   sessionData.sessionToken,
        apiKey:         sessionData.apiKey,
        callId:         sessionData.callId,
        progressURL:    sessionData.progressURL,
        websocketToken: sessionData.websocketToken.toString(16),
        callType:       sessionData.callType || "audio-video",
        callToken:      sessionData.callToken
      });
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

    





    hasVideoStream: function(callType) {
      if (callType === "incoming") {
        return this.get("callType") === "audio-video";
      }
      if (callType === "outgoing") {
        return this.get("selectedCallType") === "audio-video";
      }
      return undefined;
    },

    





    publish: function(publisher) {
      this.session.publish(publisher);
      this.set("publishedStream", true);
    },

    








    subscribe: function(stream, element, config) {
      this.session.subscribe(stream, element, config);
      this.set("subscribedStream", true);
    },

    



    streamsConnected: function() {
      return this.get("publishedStream") && this.get("subscribedStream");
    },

    










    _handleServerError: function(err) {
      switch (err.errno) {
        
        
        
        case 105:
          this.trigger("session:expired", err);
          break;
        default:
          this.trigger("session:error", err);
          break;
      }
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
    model: NotificationModel,

    




    warn: function(message) {
      this.add({level: "warning", message: message});
    },

    




    warnL10n: function(messageId) {
      this.warn(l10n.get(messageId));
    },

    




    error: function(message) {
      this.add({level: "error", message: message});
    },

    




    errorL10n: function(messageId) {
      this.error(l10n.get(messageId));
    }
  });

  return {
    ConversationModel: ConversationModel,
    NotificationCollection: NotificationCollection,
    NotificationModel: NotificationModel
  };
})(navigator.mozL10n || document.mozL10n);
