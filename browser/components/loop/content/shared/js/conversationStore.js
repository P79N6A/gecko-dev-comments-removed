





var loop = loop || {};
loop.store = (function() {

  var sharedActions = loop.shared.actions;
  var sharedUtils = loop.shared.utils;

  



  var WS_STATES = {
    
    INIT: "init",
    
    ALERTING: "alerting",
    
    TERMINATED: "terminated",
    
    
    CONNECTING: "connecting",
    
    
    HALF_CONNECTED: "half-connected",
    
    CONNECTED: "connected"
  };

  var CALL_STATES = {
    
    INIT: "cs-init",
    
    GATHER: "cs-gather",
    
    
    CONNECTING: "cs-connecting",
    
    
    ALERTING: "cs-alerting",
    
    ONGOING: "cs-ongoing",
    
    FINISHED: "cs-finished",
    
    CLOSE: "cs-close",
    
    TERMINATED: "cs-terminated"
  };


  var ConversationStore = Backbone.Model.extend({
    defaults: {
      
      callState: CALL_STATES.INIT,
      
      callStateReason: undefined,
      
      error: undefined,
      
      outgoing: undefined,
      
      calleeId: undefined,
      
      
      callType: sharedUtils.CALL_TYPES.AUDIO_VIDEO,

      
      
      callId: undefined,
      
      progressURL: undefined,
      
      websocketToken: undefined,
      
      apiKey: undefined,
      
      sessionId: undefined,
      
      sessionToken: undefined
    },

    










    initialize: function(attributes, options) {
      options = options || {};

      if (!options.dispatcher) {
        throw new Error("Missing option dispatcher");
      }
      if (!options.client) {
        throw new Error("Missing option client");
      }

      this.client = options.client;
      this.dispatcher = options.dispatcher;

      this.dispatcher.register(this, [
        "connectionFailure",
        "connectionProgress",
        "gatherCallData",
        "connectCall",
        "hangupCall",
        "cancelCall",
        "retryCall"
      ]);
    },

    





    connectionFailure: function(actionData) {
      this.set({
        callState: CALL_STATES.TERMINATED,
        callStateReason: actionData.reason
      });
    },

    





    connectionProgress: function(actionData) {
      var callState = this.get("callState");

      switch(actionData.wsState) {
        case WS_STATES.INIT: {
          if (callState === CALL_STATES.GATHER) {
            this.set({callState: CALL_STATES.CONNECTING});
          }
          break;
        }
        case WS_STATES.ALERTING: {
          this.set({callState: CALL_STATES.ALERTING});
          break;
        }
        case WS_STATES.CONNECTING:
        case WS_STATES.HALF_CONNECTED:
        case WS_STATES.CONNECTED: {
          this.set({callState: CALL_STATES.ONGOING});
          break;
        }
        default: {
          console.error("Unexpected websocket state passed to connectionProgress:",
            actionData.wsState);
        }
      }
    },

    





    gatherCallData: function(actionData) {
      this.set({
        calleeId: actionData.calleeId,
        outgoing: !!actionData.calleeId,
        callId: actionData.callId,
        callState: CALL_STATES.GATHER
      });

      if (this.get("outgoing")) {
        this._setupOutgoingCall();
      } 
    },

    






    connectCall: function(actionData) {
      this.set(actionData.sessionData);
      this._connectWebSocket();
    },

    


    hangupCall: function() {
      

      
      if (this._websocket) {
        
        this._websocket.mediaFail();
        this._ensureWebSocketDisconnected();
      }

      this.set({callState: CALL_STATES.FINISHED});
    },

    


    cancelCall: function() {
      var callState = this.get("callState");
      if (callState === CALL_STATES.TERMINATED) {
        
        this.set({callState: CALL_STATES.CLOSE});
        return;
      }

      if (callState === CALL_STATES.CONNECTING ||
          callState === CALL_STATES.ALERTING) {
        if (this._websocket) {
          
          this._websocket.cancel();
          this._ensureWebSocketDisconnected();
        }
        this.set({callState: CALL_STATES.CLOSE});
        return;
      }

      console.log("Unsupported cancel in state", callState);
    },

    


    retryCall: function() {
      var callState = this.get("callState");
      if (callState !== CALL_STATES.TERMINATED) {
        console.error("Unexpected retry in state", callState);
        return;
      }

      this.set({callState: CALL_STATES.GATHER});
      if (this.get("outgoing")) {
        this._setupOutgoingCall();
      }
    },

    



    _setupOutgoingCall: function() {
      
      this.client.setupOutgoingCall([this.get("calleeId")],
        this.get("callType"),
        function(err, result) {
          if (err) {
            console.error("Failed to get outgoing call data", err);
            this.dispatcher.dispatch(
              new sharedActions.ConnectionFailure({reason: "setup"}));
            return;
          }

          
          this.dispatcher.dispatch(
            new sharedActions.ConnectCall({sessionData: result}));
        }.bind(this)
      );
    },

    




    _connectWebSocket: function() {
      this._websocket = new loop.CallConnectionWebSocket({
        url: this.get("progressURL"),
        callId: this.get("callId"),
        websocketToken: this.get("websocketToken")
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

    


    _ensureWebSocketDisconnected: function() {
     this.stopListening(this._websocket);

      
      this._websocket.close();
      delete this._websocket;
    },

    



    _handleWebSocketProgress: function(progressData) {
      var action;

      switch(progressData.state) {
        case WS_STATES.TERMINATED: {
          action = new sharedActions.ConnectionFailure({
            reason: progressData.reason
          });
          break;
        }
        default: {
          action = new sharedActions.ConnectionProgress({
            wsState: progressData.state
          });
          break;
        }
      }

      this.dispatcher.dispatch(action);
    }
  });

  return {
    CALL_STATES: CALL_STATES,
    ConversationStore: ConversationStore,
    WS_STATES: WS_STATES
  };
})();
