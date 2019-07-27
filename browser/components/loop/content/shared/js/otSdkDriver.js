





var loop = loop || {};
loop.OTSdkDriver = (function() {

  var sharedActions = loop.shared.actions;
  var FAILURE_REASONS = loop.shared.utils.FAILURE_REASONS;

  



  var OTSdkDriver = function(options) {
      if (!options.dispatcher) {
        throw new Error("Missing option dispatcher");
      }
      if (!options.sdk) {
        throw new Error("Missing option sdk");
      }

      this.dispatcher = options.dispatcher;
      this.sdk = options.sdk;

      this.dispatcher.register(this, [
        "setupStreamElements",
        "setMute"
      ]);
  };

  OTSdkDriver.prototype = {
    






    setupStreamElements: function(actionData) {
      this.getLocalElement = actionData.getLocalElementFunc;
      this.getRemoteElement = actionData.getRemoteElementFunc;
      this.publisherConfig = actionData.publisherConfig;

      
      
      
      this.publisher = this.sdk.initPublisher(this.getLocalElement(),
        this.publisherConfig);
      this.publisher.on("accessAllowed", this._onPublishComplete.bind(this));
      this.publisher.on("accessDenied", this._onPublishDenied.bind(this));
      this.publisher.on("accessDialogOpened",
        this._onAccessDialogOpened.bind(this));
    },

    






    setMute: function(actionData) {
      if (actionData.type === "audio") {
        this.publisher.publishAudio(actionData.enabled);
      } else {
        this.publisher.publishVideo(actionData.enabled);
      }
    },

    









    connectSession: function(sessionData) {
      this.session = this.sdk.initSession(sessionData.sessionId);

      this.session.on("connectionCreated", this._onConnectionCreated.bind(this));
      this.session.on("streamCreated", this._onRemoteStreamCreated.bind(this));
      this.session.on("connectionDestroyed",
        this._onConnectionDestroyed.bind(this));
      this.session.on("sessionDisconnected",
        this._onSessionDisconnected.bind(this));

      
      this.session.connect(sessionData.apiKey, sessionData.sessionToken,
        this._onConnectionComplete.bind(this));
    },

    


    disconnectSession: function() {
      if (this.session) {
        this.session.off("streamCreated connectionDestroyed sessionDisconnected");
        this.session.disconnect();
        delete this.session;
      }
      if (this.publisher) {
        this.publisher.off("accessAllowed accessDenied accessDialogOpened");
        this.publisher.destroy();
        delete this.publisher;
      }

      
      delete this._sessionConnected;
      delete this._publisherReady;
      delete this._publishedLocalStream;
      delete this._subscribedRemoteStream;
    },

    




    _onConnectionComplete: function(error) {
      if (error) {
        console.error("Failed to complete connection", error);
        this.dispatcher.dispatch(new sharedActions.ConnectionFailure({
          reason: FAILURE_REASONS.COULD_NOT_CONNECT
        }));
        return;
      }

      this.dispatcher.dispatch(new sharedActions.ConnectedToSdkServers());
      this._sessionConnected = true;
      this._maybePublishLocalStream();
    },

    





    _onConnectionDestroyed: function(event) {
      this.dispatcher.dispatch(new sharedActions.RemotePeerDisconnected({
        peerHungup: event.reason === "clientDisconnected"
      }));
    },

    






    _onSessionDisconnected: function(event) {
      
      if (event.reason === "networkDisconnected") {
        this.dispatcher.dispatch(new sharedActions.ConnectionFailure({
          reason: FAILURE_REASONS.NETWORK_DISCONNECTED
        }));
      }
    },

    _onConnectionCreated: function(event) {
      if (this.session.connection.id === event.connection.id) {
        return;
      }

      this.dispatcher.dispatch(new sharedActions.RemotePeerConnected());
    },

    





    _onRemoteStreamCreated: function(event) {
      this.session.subscribe(event.stream,
        this.getRemoteElement(), this.publisherConfig);

      this._subscribedRemoteStream = true;
      if (this._checkAllStreamsConnected()) {
        this.dispatcher.dispatch(new sharedActions.MediaConnected());
      }
    },

    






    _onAccessDialogOpened: function(event) {
      event.preventDefault();
    },

    




    _onPublishComplete: function(event) {
      event.preventDefault();
      this._publisherReady = true;
      this._maybePublishLocalStream();
    },

    




    _onPublishDenied: function(event) {
      
      event.preventDefault();

      this.dispatcher.dispatch(new sharedActions.ConnectionFailure({
        reason: FAILURE_REASONS.MEDIA_DENIED
      }));
    },

    



    _maybePublishLocalStream: function() {
      if (this._sessionConnected && this._publisherReady) {
        
        this.session.publish(this.publisher);

        
        this._publishedLocalStream = true;
        if (this._checkAllStreamsConnected()) {
          this.dispatcher.dispatch(new sharedActions.MediaConnected());
        }
      }
    },

    



    _checkAllStreamsConnected: function() {
      return this._publishedLocalStream &&
        this._subscribedRemoteStream;
    }
  };

  return OTSdkDriver;

})();
