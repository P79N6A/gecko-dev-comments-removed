





var loop = loop || {};
loop.OTSdkDriver = (function() {

  var sharedActions = loop.shared.actions;
  var FAILURE_DETAILS = loop.shared.utils.FAILURE_DETAILS;
  var STREAM_PROPERTIES = loop.shared.utils.STREAM_PROPERTIES;
  var SCREEN_SHARE_STATES = loop.shared.utils.SCREEN_SHARE_STATES;

  



  var OTSdkDriver = function(options) {
      if (!options.dispatcher) {
        throw new Error("Missing option dispatcher");
      }
      if (!options.sdk) {
        throw new Error("Missing option sdk");
      }

      this.dispatcher = options.dispatcher;
      this.sdk = options.sdk;

      this.connections = {};

      this.dispatcher.register(this, [
        "setupStreamElements",
        "setMute",
        "startScreenShare",
        "endScreenShare"
      ]);
  };

  OTSdkDriver.prototype = {
    



    _getCopyPublisherConfig: function() {
      return _.extend({}, this.publisherConfig);
    },

    






    setupStreamElements: function(actionData) {
      this.getLocalElement = actionData.getLocalElementFunc;
      this.getScreenShareElementFunc = actionData.getScreenShareElementFunc;
      this.getRemoteElement = actionData.getRemoteElementFunc;
      this.publisherConfig = actionData.publisherConfig;

      
      
      
      this.publisher = this.sdk.initPublisher(this.getLocalElement(),
        this._getCopyPublisherConfig());
      this.publisher.on("streamCreated", this._onLocalStreamCreated.bind(this));
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

    


    startScreenShare: function() {
      this.dispatcher.dispatch(new sharedActions.ScreenSharingState({
        state: SCREEN_SHARE_STATES.PENDING
      }));

      var config = this._getCopyPublisherConfig();
      
      config.videoSource = "window";

      this.screenshare = this.sdk.initPublisher(this.getScreenShareElementFunc(),
        config);
      this.screenshare.on("accessAllowed", this._onScreenShareGranted.bind(this));
      this.screenshare.on("accessDenied", this._onScreenShareDenied.bind(this));
    },

    


    endScreenShare: function() {
      if (!this.screenshare) {
        return;
      }

      this.session.unpublish(this.screenshare);
      this.screenshare.off("accessAllowed accessDenied");
      this.screenshare.destroy();
      delete this.screenshare;
      this.dispatcher.dispatch(new sharedActions.ScreenSharingState({
        state: SCREEN_SHARE_STATES.INACTIVE
      }));
    },

    









    connectSession: function(sessionData) {
      this.session = this.sdk.initSession(sessionData.sessionId);

      this.session.on("connectionCreated", this._onConnectionCreated.bind(this));
      this.session.on("streamCreated", this._onRemoteStreamCreated.bind(this));
      this.session.on("streamDestroyed", this._onRemoteStreamDestroyed.bind(this));
      this.session.on("connectionDestroyed",
        this._onConnectionDestroyed.bind(this));
      this.session.on("sessionDisconnected",
        this._onSessionDisconnected.bind(this));
      this.session.on("streamPropertyChanged", this._onStreamPropertyChanged.bind(this));

      
      this.session.connect(sessionData.apiKey, sessionData.sessionToken,
        this._onConnectionComplete.bind(this));
    },

    


    disconnectSession: function() {
      this.endScreenShare();

      if (this.session) {
        this.session.off("streamCreated streamDestroyed connectionDestroyed " +
          "sessionDisconnected streamPropertyChanged");
        this.session.disconnect();
        delete this.session;
      }
      if (this.publisher) {
        this.publisher.off("accessAllowed accessDenied accessDialogOpened streamCreated");
        this.publisher.destroy();
        delete this.publisher;
      }

      
      delete this._sessionConnected;
      delete this._publisherReady;
      delete this._publishedLocalStream;
      delete this._subscribedRemoteStream;
      this.connections = {};
    },

    






    forceDisconnectAll: function(callback) {
      if (!this._sessionConnected) {
        callback();
        return;
      }

      var connectionNames = Object.keys(this.connections);
      if (connectionNames.length === 0) {
        callback();
        return;
      }
      var disconnectCount = 0;
      connectionNames.forEach(function(id) {
        var connection = this.connections[id];
        this.session.forceDisconnect(connection, function() {
          
          
          if (++disconnectCount === connectionNames.length) {
            callback();
          }
        });
      }, this);
    },

    




    _onConnectionComplete: function(error) {
      if (error) {
        console.error("Failed to complete connection", error);
        this.dispatcher.dispatch(new sharedActions.ConnectionFailure({
          reason: FAILURE_DETAILS.COULD_NOT_CONNECT
        }));
        return;
      }

      this.dispatcher.dispatch(new sharedActions.ConnectedToSdkServers());
      this._sessionConnected = true;
      this._maybePublishLocalStream();
    },

    





    _onConnectionDestroyed: function(event) {
      var connection = event.connection;
      if (connection && (connection.id in this.connections)) {
        delete this.connections[connection.id];
      }
      this.dispatcher.dispatch(new sharedActions.RemotePeerDisconnected({
        peerHungup: event.reason === "clientDisconnected"
      }));
    },

    






    _onSessionDisconnected: function(event) {
      var reason;
      switch (event.reason) {
        case "networkDisconnected":
          reason = FAILURE_DETAILS.NETWORK_DISCONNECTED;
          break;
        case "forceDisconnected":
          reason = FAILURE_DETAILS.EXPIRED_OR_INVALID;
          break;
        default:
          
          return;
      }

      this.dispatcher.dispatch(new sharedActions.ConnectionFailure({
        reason: reason
      }));
    },

    





    _onConnectionCreated: function(event) {
      var connection = event.connection;
      if (this.session.connection.id === connection.id) {
        return;
      }
      this.connections[connection.id] = connection;
      this.dispatcher.dispatch(new sharedActions.RemotePeerConnected());
    },

    







    _handleRemoteScreenShareCreated: function(stream) {
      if (!this.getScreenShareElementFunc) {
        return;
      }

      
      this.dispatcher.dispatch(new sharedActions.ReceivingScreenShare({
        receiving: true
      }));

      var remoteElement = this.getScreenShareElementFunc();

      this.session.subscribe(stream,
        remoteElement, this._getCopyPublisherConfig());
    },

    





    _onRemoteStreamCreated: function(event) {
      if (event.stream[STREAM_PROPERTIES.HAS_VIDEO]) {
        this.dispatcher.dispatch(new sharedActions.VideoDimensionsChanged({
          isLocal: false,
          videoType: event.stream.videoType,
          dimensions: event.stream[STREAM_PROPERTIES.VIDEO_DIMENSIONS]
        }));
      }

      if (event.stream.videoType === "screen") {
        this._handleRemoteScreenShareCreated(event.stream);
        return;
      }

      var remoteElement = this.getRemoteElement();

      this.session.subscribe(event.stream,
        remoteElement, this._getCopyPublisherConfig());

      this._subscribedRemoteStream = true;
      if (this._checkAllStreamsConnected()) {
        this.dispatcher.dispatch(new sharedActions.MediaConnected());
      }
    },

    





    _onLocalStreamCreated: function(event) {
      if (event.stream[STREAM_PROPERTIES.HAS_VIDEO]) {
        this.dispatcher.dispatch(new sharedActions.VideoDimensionsChanged({
          isLocal: true,
          videoType: event.stream.videoType,
          dimensions: event.stream[STREAM_PROPERTIES.VIDEO_DIMENSIONS]
        }));
      }
    },


    





    _onRemoteStreamDestroyed: function(event) {
      if (event.stream.videoType !== "screen") {
        return;
      }

      
      
      this.dispatcher.dispatch(new sharedActions.ReceivingScreenShare({
        receiving: false
      }));
    },

    






    _onAccessDialogOpened: function(event) {
      event.preventDefault();
    },

    




    _onPublishComplete: function(event) {
      event.preventDefault();
      this._publisherReady = true;

      this.dispatcher.dispatch(new sharedActions.GotMediaPermission());

      this._maybePublishLocalStream();
    },

    




    _onPublishDenied: function(event) {
      
      event.preventDefault();

      this.dispatcher.dispatch(new sharedActions.ConnectionFailure({
        reason: FAILURE_DETAILS.MEDIA_DENIED
      }));
    },

    


    _onStreamPropertyChanged: function(event) {
      if (event.changedProperty == STREAM_PROPERTIES.VIDEO_DIMENSIONS) {
        this.dispatcher.dispatch(new sharedActions.VideoDimensionsChanged({
          isLocal: event.stream.connection.id == this.session.connection.id,
          videoType: event.stream.videoType,
          dimensions: event.stream[STREAM_PROPERTIES.VIDEO_DIMENSIONS]
        }));
      }
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
    },

    


    _onScreenShareGranted: function() {
      this.session.publish(this.screenshare);
      this.dispatcher.dispatch(new sharedActions.ScreenSharingState({
        state: SCREEN_SHARE_STATES.ACTIVE
      }));
    },

    


    _onScreenShareDenied: function() {
      this.dispatcher.dispatch(new sharedActions.ScreenSharingState({
        state: SCREEN_SHARE_STATES.INACTIVE
      }));
    }
  };

  return OTSdkDriver;

})();
