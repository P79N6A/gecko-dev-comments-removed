





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

    this._isDesktop = !!options.isDesktop;

    if (this._isDesktop) {
      if (!options.mozLoop) {
        throw new Error("Missing option mozLoop");
      }
      this.mozLoop = options.mozLoop;
    }

    this.connections = {};

    
    
    this._metrics = {
      connections: 0,
      sendStreams: 0,
      recvStreams: 0
    };

    this.dispatcher.register(this, [
      "setupStreamElements",
      "setMute"
    ]);

    
    
    
    
    
    this._debugTwoWayMediaTelemetry =
      loop.shared.utils.getBoolPreference("debug.twoWayMediaTelemetry");

    




    if (this._isDesktop && !window.MediaStreamTrack.getSources) {
      
      
      
      window.MediaStreamTrack.getSources = function(callback) {
        callback([{kind: "audio"}, {kind: "video"}]);
      };
    }
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

      this.sdk.on("exception", this._onOTException.bind(this));

      
      
      
      this._publishLocalStreams();
    },

    



    _publishLocalStreams: function() {
      this.publisher = this.sdk.initPublisher(this.getLocalElement(),
        this._getCopyPublisherConfig());
      this.publisher.on("streamCreated", this._onLocalStreamCreated.bind(this));
      this.publisher.on("streamDestroyed", this._onLocalStreamDestroyed.bind(this));
      this.publisher.on("accessAllowed", this._onPublishComplete.bind(this));
      this.publisher.on("accessDenied", this._onPublishDenied.bind(this));
      this.publisher.on("accessDialogOpened",
        this._onAccessDialogOpened.bind(this));
    },

    



    retryPublishWithoutVideo: function() {
      window.MediaStreamTrack.getSources = function(callback) {
        callback([{kind: "audio"}]);
      };
      this._publishLocalStreams();
    },

    






    setMute: function(actionData) {
      if (actionData.type === "audio") {
        this.publisher.publishAudio(actionData.enabled);
      } else {
        this.publisher.publishVideo(actionData.enabled);
      }
    },

    














    startScreenShare: function(options) {
      
      
      if (options.videoSource === "browser") {
        this._windowId = options.constraints.browserWindow;
      }

      var config = _.extend(this._getCopyPublisherConfig(), options);

      this.screenshare = this.sdk.initPublisher(this.getScreenShareElementFunc(),
        config);
      this.screenshare.on("accessAllowed", this._onScreenShareGranted.bind(this));
      this.screenshare.on("accessDenied", this._onScreenShareDenied.bind(this));
      this.screenshare.on("streamCreated", this._onScreenShareStreamCreated.bind(this));

      this._noteSharingState(options.videoSource, true);
    },

    




    switchAcquiredWindow: function(windowId) {
      if (windowId === this._windowId) {
        return;
      }

      this._windowId = windowId;
      this.screenshare._.switchAcquiredWindow(windowId);
    },

    





    endScreenShare: function() {
      if (!this.screenshare) {
        return false;
      }

      this._notifyMetricsEvent("Publisher.streamDestroyed");

      this.session.unpublish(this.screenshare);
      this.screenshare.off("accessAllowed accessDenied streamCreated");
      this.screenshare.destroy();
      delete this.screenshare;
      this._noteSharingState(this._windowId ? "browser" : "window", false);
      delete this._windowId;
      return true;
    },

    














    connectSession: function(sessionData) {
      this.session = this.sdk.initSession(sessionData.sessionId);

      this._sendTwoWayMediaTelemetry = !!sessionData.sendTwoWayMediaTelemetry;
      this._setTwoWayMediaStartTime(this.CONNECTION_START_TIME_UNINITIALIZED);

      this.session.on("sessionDisconnected",
        this._onSessionDisconnected.bind(this));
      this.session.on("connectionCreated", this._onConnectionCreated.bind(this));
      this.session.on("connectionDestroyed",
        this._onConnectionDestroyed.bind(this));
      this.session.on("streamCreated", this._onRemoteStreamCreated.bind(this));
      this.session.on("streamDestroyed", this._onRemoteStreamDestroyed.bind(this));
      this.session.on("streamPropertyChanged", this._onStreamPropertyChanged.bind(this));

      
      this.session.connect(sessionData.apiKey, sessionData.sessionToken,
        this._onSessionConnectionCompleted.bind(this));
    },

    


    disconnectSession: function() {
      this.endScreenShare();

      if (this.session) {
        this.session.off("sessionDisconnected streamCreated streamDestroyed connectionCreated connectionDestroyed streamPropertyChanged");
        this.session.disconnect();
        delete this.session;

        this._notifyMetricsEvent("Session.connectionDestroyed", "local");
      }
      if (this.publisher) {
        this.publisher.off("accessAllowed accessDenied accessDialogOpened streamCreated");
        this.publisher.destroy();
        delete this.publisher;
      }

      this._noteConnectionLengthIfNeeded(this._getTwoWayMediaStartTime(), performance.now());

      
      delete this._sessionConnected;
      delete this._publisherReady;
      delete this._publishedLocalStream;
      delete this._subscribedRemoteStream;
      this.connections = {};
      this._setTwoWayMediaStartTime(this.CONNECTION_START_TIME_UNINITIALIZED);
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

    




    _onSessionConnectionCompleted: function(error) {
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

      this._notifyMetricsEvent("Session.connectionDestroyed", "peer");

      this._noteConnectionLengthIfNeeded(this._getTwoWayMediaStartTime(), performance.now());

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

      this._noteConnectionLengthIfNeeded(this._getTwoWayMediaStartTime(),
        performance.now());
      this.dispatcher.dispatch(new sharedActions.ConnectionFailure({
        reason: reason
      }));
    },

    





    _onConnectionCreated: function(event) {
      var connection = event.connection;
      if (this.session.connection.id === connection.id) {
        
        this._notifyMetricsEvent("Session.connectionCreated", "local");
        return;
      }
      this.connections[connection.id] = connection;
      this._notifyMetricsEvent("Session.connectionCreated", "peer");
      this.dispatcher.dispatch(new sharedActions.RemotePeerConnected());
    },

    



    _getConnectionState: function() {
      if (this._metrics.sendStreams) {
        return this._metrics.recvStreams ? "sendrecv" : "sending";
      }

      if (this._metrics.recvStreams) {
        return "receiving";
      }

      return "starting";
    },

    







    _notifyMetricsEvent: function(eventName, clientType) {
      if (!eventName) {
        return;
      }

      var state;

      
      
      
      switch (eventName) {
        case "Session.connectionCreated":
          this._metrics.connections++;
          if (clientType === "local") {
            state = "waiting";
          }
          break;
        case "Session.connectionDestroyed":
          this._metrics.connections--;
          if (clientType === "local") {
            
            
            return;
          } else if (!this._metrics.connections) {
            state = "waiting";
          }
          break;
        case "Publisher.streamCreated":
          this._metrics.sendStreams++;
          break;
        case "Publisher.streamDestroyed":
          this._metrics.sendStreams--;
          break;
        case "Session.streamCreated":
          this._metrics.recvStreams++;
          break;
        case "Session.streamDestroyed":
          this._metrics.recvStreams--;
          break;
        default:
          console.error("Unexpected event name", eventName);
          return;
      }
      if (!state) {
        state = this._getConnectionState();
      }

      this.dispatcher.dispatch(new sharedActions.ConnectionStatus({
        event: eventName,
        state: state,
        connections: this._metrics.connections,
        sendStreams: this._metrics.sendStreams,
        recvStreams: this._metrics.recvStreams
      }));
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
      this._notifyMetricsEvent("Session.streamCreated");

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
        this._setTwoWayMediaStartTime(performance.now());
        this.dispatcher.dispatch(new sharedActions.MediaConnected());
      }
    },

    





    _onLocalStreamCreated: function(event) {
      this._notifyMetricsEvent("Publisher.streamCreated");

      if (event.stream[STREAM_PROPERTIES.HAS_VIDEO]) {
        this.dispatcher.dispatch(new sharedActions.VideoDimensionsChanged({
          isLocal: true,
          videoType: event.stream.videoType,
          dimensions: event.stream[STREAM_PROPERTIES.VIDEO_DIMENSIONS]
        }));
      }
    },

    





    __twoWayMediaStartTime: undefined,

    



    CONNECTION_START_TIME_UNINITIALIZED: -1,

    



    CONNECTION_START_TIME_ALREADY_NOTED: -2,

    















    _setTwoWayMediaStartTime: function(start) {
      if (!this._sendTwoWayMediaTelemetry) {
        return;
      }

      this.__twoWayMediaStartTime = start;
      if (this._debugTwoWayMediaTelemetry) {
        console.log("Loop Telemetry: noted two-way connection start, " +
                    "start time in ms:", start);
      }
    },
    _getTwoWayMediaStartTime: function() {
      return this.__twoWayMediaStartTime;
    },

    





    _onRemoteStreamDestroyed: function(event) {
      this._notifyMetricsEvent("Session.streamDestroyed");

      if (event.stream.videoType !== "screen") {
        return;
      }

      
      
      this.dispatcher.dispatch(new sharedActions.ReceivingScreenShare({
        receiving: false
      }));
    },

    


    _onLocalStreamDestroyed: function() {
      this._notifyMetricsEvent("Publisher.streamDestroyed");
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

    _onOTException: function(event) {
      if (event.code === OT.ExceptionCodes.UNABLE_TO_PUBLISH &&
          event.message === "GetUserMedia") {
        
        
        if (this.publisher) {
          this.publisher.off("accessAllowed accessDenied accessDialogOpened streamCreated");
          this.publisher.destroy();
          delete this.publisher;
        }
        this.dispatcher.dispatch(new sharedActions.ConnectionFailure({
          reason: FAILURE_DETAILS.UNABLE_TO_PUBLISH_MEDIA
        }));
      }
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
          this._setTwoWayMediaStartTime(performance.now());
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
    },

    


    _onScreenShareStreamCreated: function() {
      this._notifyMetricsEvent("Publisher.streamCreated");
    },

    







    







    _connectionLengthNotedCalls: 0,

    







    _noteConnectionLength: function(callLengthSeconds) {
      var buckets = this.mozLoop.TWO_WAY_MEDIA_CONN_LENGTH;

      var bucket = buckets.SHORTER_THAN_10S;
      if (callLengthSeconds >= 10 && callLengthSeconds <= 30) {
        bucket = buckets.BETWEEN_10S_AND_30S;
      } else if (callLengthSeconds > 30 && callLengthSeconds <= 300) {
        bucket = buckets.BETWEEN_30S_AND_5M;
      } else if (callLengthSeconds > 300) {
        bucket = buckets.MORE_THAN_5M;
      }

      this.mozLoop.telemetryAddValue("LOOP_TWO_WAY_MEDIA_CONN_LENGTH", bucket);
      this._setTwoWayMediaStartTime(this.CONNECTION_START_TIME_ALREADY_NOTED);

      this._connectionLengthNotedCalls++;
      if (this._debugTwoWayMediaTelemetry) {
        console.log('Loop Telemetry: noted two-way media connection ' +
          'in bucket: ', bucket);
      }
    },

    








    _noteConnectionLengthIfNeeded: function(startTime, endTime) {
      if (!this._sendTwoWayMediaTelemetry) {
        return;
      }

      if (startTime == this.CONNECTION_START_TIME_ALREADY_NOTED ||
          startTime == this.CONNECTION_START_TIME_UNINITIALIZED ||
          startTime > endTime) {
        if (this._debugTwoWayMediaTelemetry) {
          console.log("_noteConnectionLengthIfNeeded called with " +
            " invalid params, either the calls were never" +
            " connected or there is a bug; startTime:", startTime,
            "endTime:", endTime);
        }
        return;
      }

      var callLengthSeconds = (endTime - startTime) / 1000;
      this._noteConnectionLength(callLengthSeconds);
    },

    



    _debugTwoWayMediaTelemetry: false,

    









    _noteSharingState: function(type, enabled) {
      if (!this.mozLoop) {
        return;
      }

      var bucket = this.mozLoop.SHARING_STATE_CHANGE[type.toUpperCase() + "_" +
        (enabled ? "ENABLED" : "DISABLED")];
      if (!bucket) {
        console.error("No sharing state bucket found for '" + type + "'");
        return;
      }

      this.mozLoop.telemetryAddValue("LOOP_SHARING_STATE_CHANGE", bucket);
    }
  };

  return OTSdkDriver;

})();
