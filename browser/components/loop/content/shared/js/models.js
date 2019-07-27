





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
      sessionId:    undefined,     
      sessionToken: undefined,     
      sessionType:  undefined,     
      apiKey:       undefined,     
      callId:       undefined,     
      progressURL:  undefined,     
      websocketToken: undefined,   
                                   
                                   
      callType:     undefined,     
                                   
      selectedCallType: "audio-video", 
                                       
      callToken:    undefined,     
      callUrl:      undefined,     
                                   
      subscribedStream: false,     
                                   
      publishedStream: false       
                                   
    },

    



    sdk: undefined,

    



    session: undefined,

    










    initialize: function(attributes, options) {
      options = options || {};
      if (!options.sdk) {
        throw new Error("missing required sdk");
      }
      this.sdk = options.sdk;

      
      
      if (loop.shared.utils.getBoolPreference("debug.sdk")) {
        this.sdk.setLogLevel(this.sdk.DEBUG);
      }
    },

    


    accepted: function() {
      this.trigger("call:accepted");
    },

    





    setupOutgoingCall: function(selectedCallType) {
      if (selectedCallType) {
        this.set("selectedCallType", selectedCallType);
      }
      this.trigger("call:outgoing:setup");
    },

    





    outgoing: function(sessionData) {
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
        sessionId:       sessionData.sessionId,
        sessionToken:    sessionData.sessionToken,
        sessionType:     sessionData.sessionType,
        apiKey:          sessionData.apiKey,
        callId:          sessionData.callId,
        callerId:        sessionData.callerId,
        urlCreationDate: sessionData.urlCreationDate,
        progressURL:     sessionData.progressURL,
        websocketToken:  sessionData.websocketToken.toString(16),
        callType:        sessionData.callType || "audio-video",
        callToken:       sessionData.callToken,
        callUrl:         sessionData.callUrl
      });
    },

    


    startSession: function() {
      if (!this.isSessionReady()) {
        throw new Error("Can't start session as it's not ready");
      }
      this.set({
        publishedStream: false,
        subscribedStream: false
      });

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
      this.set({
        publishedStream: false,
        subscribedStream: false,
        ongoing: false
      }).once("session:ended", this.stopListening, this);
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

    


    _removeScheme: function(url) {
      if (!url) {
        return "";
      }
      return url.replace(/^https?:\/\//, "");
    },

    


    getCallIdentifier: function() {
      return this.get("callerId") || this._removeScheme(this.get("callUrl"));
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
      details: "",
      detailsButtonLabel: "",
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

    







    errorL10n: function(messageId, l10nProps) {
      this.error(l10n.get(messageId, l10nProps));
    }
  });

  return {
    ConversationModel: ConversationModel,
    NotificationCollection: NotificationCollection,
    NotificationModel: NotificationModel
  };
})(navigator.mozL10n || document.mozL10n);
