



var loop = loop || {};
loop.store = loop.store || {};

(function() {
  "use strict";

  var sharedActions = loop.shared.actions;
  var CALL_TYPES = loop.shared.utils.CALL_TYPES;
  var REST_ERRNOS = loop.shared.utils.REST_ERRNOS;
  var FAILURE_DETAILS = loop.shared.utils.FAILURE_DETAILS;

  



  var WS_STATES = loop.store.WS_STATES = {
    
    INIT: "init",
    
    ALERTING: "alerting",
    
    TERMINATED: "terminated",
    
    
    CONNECTING: "connecting",
    
    
    HALF_CONNECTED: "half-connected",
    
    CONNECTED: "connected"
  };

  var CALL_STATES = loop.store.CALL_STATES = {
    
    INIT: "cs-init",
    
    GATHER: "cs-gather",
    
    
    CONNECTING: "cs-connecting",
    
    
    ALERTING: "cs-alerting",
    
    ONGOING: "cs-ongoing",
    
    FINISHED: "cs-finished",
    
    CLOSE: "cs-close",
    
    TERMINATED: "cs-terminated"
  };

  









  loop.store.ConversationStore = loop.store.createStore({
    
    
    actions: [
      "setupWindowData"
    ],

    getInitialStoreState: function() {
      return {
        
        windowId: undefined,
        
        callState: CALL_STATES.INIT,
        
        callStateReason: undefined,
        
        outgoing: undefined,
        
        contact: undefined,
        
        
        callType: CALL_TYPES.AUDIO_VIDEO,
        
        emailLink: undefined,

        
        
        callId: undefined,
        
        callerId: undefined,
        
        mediaConnected: false,
        
        progressURL: undefined,
        
        websocketToken: undefined,
        
        apiKey: undefined,
        
        sessionId: undefined,
        
        sessionToken: undefined,
        
        audioMuted: false,
        
        videoMuted: false,
        remoteVideoEnabled: false
      };
    },

    




    initialize: function(options) {
      options = options || {};

      if (!options.client) {
        throw new Error("Missing option client");
      }
      if (!options.sdkDriver) {
        throw new Error("Missing option sdkDriver");
      }
      if (!options.mozLoop) {
        throw new Error("Missing option mozLoop");
      }

      this.client = options.client;
      this.sdkDriver = options.sdkDriver;
      this.mozLoop = options.mozLoop;
      this._isDesktop = options.isDesktop || false;
    },

    





    connectionFailure: function(actionData) {
      




      if (this._isDesktop &&
          actionData.reason === FAILURE_DETAILS.UNABLE_TO_PUBLISH_MEDIA &&
          this.getStoreState().videoMuted === false) {
        
        
        this.setStoreState({videoMuted: true});
        this.sdkDriver.retryPublishWithoutVideo();
        return;
      }

      this._endSession();
      this.setStoreState({
        callState: CALL_STATES.TERMINATED,
        callStateReason: actionData.reason
      });
    },

    





    connectionProgress: function(actionData) {
      var state = this.getStoreState();

      switch(actionData.wsState) {
        case WS_STATES.INIT: {
          if (state.callState === CALL_STATES.GATHER) {
            this.setStoreState({callState: CALL_STATES.CONNECTING});
          }
          break;
        }
        case WS_STATES.ALERTING: {
          this.setStoreState({callState: CALL_STATES.ALERTING});
          break;
        }
        case WS_STATES.CONNECTING: {
          if (state.outgoing) {
            
            
            
            this._startCallConnection();
          } else if (state.callState !== CALL_STATES.ONGOING) {
            console.error("Websocket unexpectedly changed to next state whilst waiting for call acceptance.");
            
            this.declineCall(new sharedActions.DeclineCall({blockCaller: false}));
          }
          break;
        }
        case WS_STATES.HALF_CONNECTED:
        case WS_STATES.CONNECTED: {
          if (this.getStoreState("callState") !== CALL_STATES.ONGOING) {
            console.error("Unexpected websocket state received in wrong callState");
            this.setStoreState({callState: CALL_STATES.ONGOING});
          }
          break;
        }
        default: {
          console.error("Unexpected websocket state passed to connectionProgress:",
            actionData.wsState);
        }
      }
    },

    setupWindowData: function(actionData) {
      var windowType = actionData.type;
      if (windowType !== "outgoing" &&
          windowType !== "incoming") {
        
        return;
      }

      this.dispatcher.register(this, [
        "connectionFailure",
        "connectionProgress",
        "acceptCall",
        "declineCall",
        "connectCall",
        "hangupCall",
        "remotePeerDisconnected",
        "cancelCall",
        "retryCall",
        "mediaConnected",
        "setMute",
        "fetchRoomEmailLink",
        "localVideoEnabled",
        "remoteVideoDisabled",
        "remoteVideoEnabled",
        "windowUnload"
      ]);

      this.setStoreState({
        apiKey: actionData.apiKey,
        callerId: actionData.callerId,
        callId: actionData.callId,
        callState: CALL_STATES.GATHER,
        callType: actionData.callType,
        contact: actionData.contact,
        outgoing: windowType === "outgoing",
        progressURL: actionData.progressURL,
        sessionId: actionData.sessionId,
        sessionToken: actionData.sessionToken,
        videoMuted: actionData.callType === CALL_TYPES.AUDIO_ONLY,
        websocketToken: actionData.websocketToken,
        windowId: actionData.windowId
      });

      if (this.getStoreState("outgoing")) {
        this._setupOutgoingCall();
      } else {
        this._setupIncomingCall();
      }
    },

    



    _startCallConnection: function() {
      var state = this.getStoreState();

      this.sdkDriver.connectSession({
        apiKey: state.apiKey,
        sessionId: state.sessionId,
        sessionToken: state.sessionToken,
        sendTwoWayMediaTelemetry: state.outgoing 
      });
      this.mozLoop.addConversationContext(
        state.windowId,
        state.sessionId,
        state.callId);
      this.setStoreState({callState: CALL_STATES.ONGOING});
    },

    




    acceptCall: function(actionData) {
      if (this.getStoreState("outgoing")) {
        console.error("Received AcceptCall action in outgoing call state");
        return;
      }

      this.setStoreState({
        callType: actionData.callType,
        videoMuted: actionData.callType === CALL_TYPES.AUDIO_ONLY
      });

      this._websocket.accept();

      this._startCallConnection();
    },

    




    declineCall: function(actionData) {
      if (actionData.blockCaller) {
        this.mozLoop.calls.blockDirectCaller(this.getStoreState("callerId"),
          function(err) {
            
            
            
            console.log(err.fileName + ":" + err.lineNumber + ": " + err.message);
          });
      }

      this._websocket.decline();

      
      this._endSession();
      this.setStoreState({callState: CALL_STATES.CLOSE});
    },

    






    connectCall: function(actionData) {
      this.setStoreState(actionData.sessionData);
      this._connectWebSocket();
    },

    


    hangupCall: function() {
      if (this._websocket) {
        
        this._websocket.mediaFail();
      }

      this._endSession();
      this.setStoreState({callState: CALL_STATES.FINISHED});
    },

    




    remotePeerDisconnected: function(actionData) {
      this._endSession();

      
      
      if (actionData.peerHungup) {
        this.setStoreState({callState: CALL_STATES.FINISHED});
      } else {
        this.setStoreState({
          callState: CALL_STATES.TERMINATED,
          callStateReason: "peerNetworkDisconnected"
        });
      }
    },

    




    cancelCall: function() {
      if (this.getStoreState("outgoing")) {
        var callState = this.getStoreState("callState");
        if (this._websocket &&
            (callState === CALL_STATES.CONNECTING ||
             callState === CALL_STATES.ALERTING)) {
          
          this._websocket.cancel();
        }
      }

      this._endSession();
      this.setStoreState({callState: CALL_STATES.CLOSE});
    },

    


    retryCall: function() {
      var callState = this.getStoreState("callState");
      if (callState !== CALL_STATES.TERMINATED) {
        console.error("Unexpected retry in state", callState);
        return;
      }

      this.setStoreState({callState: CALL_STATES.GATHER});
      if (this.getStoreState("outgoing")) {
        this._setupOutgoingCall();
      }
    },

    


    mediaConnected: function() {
      this._websocket.mediaUp();
      this.setStoreState({mediaConnected: true});
    },

    




    setMute: function(actionData) {
      var newState = {};
      newState[actionData.type + "Muted"] = !actionData.enabled;
      this.setStoreState(newState);
    },

    



    fetchRoomEmailLink: function(actionData) {
      this.mozLoop.rooms.create({
        roomName: actionData.roomName,
        roomOwner: actionData.roomOwner,
        maxSize: loop.store.MAX_ROOM_CREATION_SIZE,
        expiresIn: loop.store.DEFAULT_EXPIRES_IN
      }, function(err, createdRoomData) {
        var buckets = this.mozLoop.ROOM_CREATE;
        if (err) {
          this.trigger("error:emailLink");
          this.mozLoop.telemetryAddValue("LOOP_ROOM_CREATE", buckets.CREATE_FAIL);
          return;
        }
        this.setStoreState({"emailLink": createdRoomData.roomUrl});
        this.mozLoop.telemetryAddValue("LOOP_ROOM_CREATE", buckets.CREATE_SUCCESS);
      }.bind(this));
    },

    




    remoteVideoEnabled: function(actionData) {
      this.setStoreState({
        remoteVideoEnabled: true,
        remoteSrcVideoObject: actionData.srcVideoObject
      });
    },

    




    remoteVideoDisabled: function(actionData) {
      this.setStoreState({
        remoteVideoEnabled: false,
        remoteSrcVideoObject: undefined});
    },

    








    localVideoEnabled: function(actionData) {
      this.setStoreState({
        localSrcVideoObject: actionData.srcVideoObject
      });
    },

    




    windowUnload: function() {
      if (!this.getStoreState("outgoing") &&
          this.getStoreState("callState") === CALL_STATES.ALERTING &&
          this._websocket) {
        this._websocket.decline();
      }

      this._endSession();
    },

    




    _setupIncomingCall: function() {
      this._connectWebSocket();
    },

    



    _setupOutgoingCall: function() {
      var contactAddresses = [];
      var contact = this.getStoreState("contact");

      this.mozLoop.calls.setCallInProgress(this.getStoreState("windowId"));

      function appendContactValues(property, strip) {
        if (contact.hasOwnProperty(property)) {
          contact[property].forEach(function(item) {
            if (strip) {
              contactAddresses.push(item.value
                .replace(/^(\+)?(.*)$/g, function(m, prefix, number) {
                  return (prefix || "") + number.replace(/[\D]+/g, "");
                }));
            } else {
              contactAddresses.push(item.value);
            }
          });
        }
      }

      appendContactValues("email");
      appendContactValues("tel", true);

      this.client.setupOutgoingCall(contactAddresses,
        this.getStoreState("callType"),
        function(err, result) {
          if (err) {
            console.error("Failed to get outgoing call data", err);
            var failureReason = "setup";
            if (err.errno == REST_ERRNOS.USER_UNAVAILABLE) {
              failureReason = REST_ERRNOS.USER_UNAVAILABLE;
            }
            this.dispatcher.dispatch(
              new sharedActions.ConnectionFailure({reason: failureReason}));
            return;
          }

          
          this.dispatcher.dispatch(
            new sharedActions.ConnectCall({sessionData: result}));
        }.bind(this)
      );
    },

    




    _connectWebSocket: function() {
      this._websocket = new loop.CallConnectionWebSocket({
        url: this.getStoreState("progressURL"),
        callId: this.getStoreState("callId"),
        websocketToken: this.getStoreState("websocketToken")
      });

      this._websocket.promiseConnect().then(
        function(progressState) {
          this.dispatcher.dispatch(new sharedActions.ConnectionProgress({
            
            
            wsState: progressState
          }));
        }.bind(this),
        function(error) {
          console.error("Websocket failed to connect", error);
          this.dispatcher.dispatch(new sharedActions.ConnectionFailure({
            reason: "websocket-setup"
          }));
        }.bind(this)
      );

      this.listenTo(this._websocket, "progress", this._handleWebSocketProgress);
    },

    


    _endSession: function(nextState) {
      this.sdkDriver.disconnectSession();
      if (this._websocket) {
        this.stopListening(this._websocket);

        
        this._websocket.close();
        delete this._websocket;
      }

      this.mozLoop.calls.clearCallInProgress(
        this.getStoreState("windowId"));
    },

    













    _handleWebSocketStateTerminated: function(progressData, previousState) {
      if (this.getStoreState("outgoing") ||
          (previousState !== WS_STATES.INIT &&
           previousState !== WS_STATES.ALERTING)) {
        
        this.dispatcher.dispatch(new sharedActions.ConnectionFailure({
          reason: progressData.reason
        }));
        return;
      }

      this.dispatcher.dispatch(new sharedActions.CancelCall());
    },

    






    _handleWebSocketProgress: function(progressData, previousState) {
      switch(progressData.state) {
        case WS_STATES.TERMINATED: {
          this._handleWebSocketStateTerminated(progressData, previousState);
          break;
        }
        default: {
          this.dispatcher.dispatch(new sharedActions.ConnectionProgress({
            wsState: progressData.state
          }));
          break;
        }
      }
    }
  });
})();
