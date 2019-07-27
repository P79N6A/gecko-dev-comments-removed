





var loop = loop || {};
loop.store = (function() {

  var sharedActions = loop.shared.actions;
  var sharedUtils = loop.shared.utils;

  var CALL_STATES = {
    
    INIT: "init",
    
    GATHER: "gather",
    
    
    CONNECTING: "connecting",
    
    
    ALERTING: "alerting",
    
    TERMINATED: "terminated"
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
        "connectCall"
      ]);
    },

    





    connectionFailure: function(actionData) {
      this.set({
        callState: CALL_STATES.TERMINATED,
        callStateReason: actionData.reason
      });
    },

    





    connectionProgress: function(actionData) {
      
      if (actionData.state === "alerting" &&
          (this.get("callState") === CALL_STATES.CONNECTING ||
           this.get("callState") === CALL_STATES.GATHER)) {
        this.set({
          callState: CALL_STATES.ALERTING
        });
      }
      if (actionData.state === "connecting" &&
          this.get("callState") === CALL_STATES.GATHER) {
        this.set({
          callState: CALL_STATES.CONNECTING
        });
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
        function() {
          this.dispatcher.dispatch(new sharedActions.ConnectionProgress({
            
            
            state: "connecting"
          }));
        }.bind(this),
        function(error) {
          console.error("Websocket failed to connect", error);
          this.dispatcher.dispatch(new sharedActions.ConnectionFailure({
            reason: "websocket-setup"
          }));
        }.bind(this)
      );

      this._websocket.on("progress", this._handleWebSocketProgress, this);
    },

    



    _handleWebSocketProgress: function(progressData) {
      var action;

      switch(progressData.state) {
        case "terminated":
          action = new sharedActions.ConnectionFailure({
            reason: progressData.reason
          });
          break;
        case "alerting":
          action = new sharedActions.ConnectionProgress({
            state: progressData.state
          });
          break;
        default:
          console.warn("Received unexpected state in _handleWebSocketProgress", progressData.state);
          return;
      }

      this.dispatcher.dispatch(action);
    }
  });

  return {
    CALL_STATES: CALL_STATES,
    ConversationStore: ConversationStore
  };
})();
