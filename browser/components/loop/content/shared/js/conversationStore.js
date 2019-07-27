





var loop = loop || {};
loop.store = loop.store || {};

loop.store.ConversationStore = (function() {
  var sharedActions = loop.shared.actions;
  var CALL_TYPES = loop.shared.utils.CALL_TYPES;

  



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

  
  var ConversationStore = Backbone.Model.extend({
    defaults: {
      
      windowId: undefined,
      
      callState: CALL_STATES.INIT,
      
      callStateReason: undefined,
      
      error: undefined,
      
      outgoing: undefined,
      
      contact: undefined,
      
      
      callType: CALL_TYPES.AUDIO_VIDEO,

      
      
      callId: undefined,
      
      progressURL: undefined,
      
      websocketToken: undefined,
      
      apiKey: undefined,
      
      sessionId: undefined,
      
      sessionToken: undefined,
      
      audioMuted: false,
      
      videoMuted: false
    },

    










    initialize: function(attributes, options) {
      options = options || {};

      if (!options.dispatcher) {
        throw new Error("Missing option dispatcher");
      }
      if (!options.client) {
        throw new Error("Missing option client");
      }
      if (!options.sdkDriver) {
        throw new Error("Missing option sdkDriver");
      }

      this.client = options.client;
      this.dispatcher = options.dispatcher;
      this.sdkDriver = options.sdkDriver;

      
      
      
      
      this.dispatcher.register(this, [
        "setupWindowData"
      ]);
    },

    





    connectionFailure: function(actionData) {
      this._endSession();
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
        case WS_STATES.CONNECTING: {
          this.sdkDriver.connectSession({
            apiKey: this.get("apiKey"),
            sessionId: this.get("sessionId"),
            sessionToken: this.get("sessionToken")
          });
          this.set({callState: CALL_STATES.ONGOING});
          break;
        }
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

    setupWindowData: function(actionData) {
      var windowType = actionData.type;
      if (windowType !== "outgoing" &&
          windowType !== "incoming") {
        
        return;
      }

      this.dispatcher.register(this, [
        "connectionFailure",
        "connectionProgress",
        "connectCall",
        "hangupCall",
        "remotePeerDisconnected",
        "cancelCall",
        "retryCall",
        "mediaConnected",
        "setMute",
        "fetchEmailLink"
      ]);

      this.set({
        contact: actionData.contact,
        outgoing: windowType === "outgoing",
        windowId: actionData.windowId,
        callType: actionData.callType,
        callState: CALL_STATES.GATHER,
        videoMuted: actionData.callType === CALL_TYPES.AUDIO_ONLY
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
      }

      this._endSession();
      this.set({callState: CALL_STATES.FINISHED});
    },

    




    remotePeerDisconnected: function(actionData) {
      this._endSession();

      
      
      if (actionData.peerHungup) {
        this.set({callState: CALL_STATES.FINISHED});
      } else {
        this.set({
          callState: CALL_STATES.TERMINATED,
          callStateReason: "peerNetworkDisconnected"
        });
      }
    },

    


    cancelCall: function() {
      var callState = this.get("callState");
      if (this._websocket &&
          (callState === CALL_STATES.CONNECTING ||
           callState === CALL_STATES.ALERTING)) {
         
        this._websocket.cancel();
      }

      this._endSession();
      this.set({callState: CALL_STATES.CLOSE});
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

    


    mediaConnected: function() {
      this._websocket.mediaUp();
    },

    




    setMute: function(actionData) {
      var muteType = actionData.type + "Muted";
      this.set(muteType, !actionData.enabled);
    },

    



    fetchEmailLink: function() {
      
      
      this.client.requestCallUrl("", function(err, callUrlData) {
        if (err) {
          this.trigger("error:emailLink");
          return;
        }
        this.set("emailLink", callUrlData.callUrl);
      }.bind(this));
    },

    



    _setupOutgoingCall: function() {
      var contactAddresses = [];
      var contact = this.get("contact");

      navigator.mozLoop.calls.setCallInProgress(this.get("windowId"));

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

    


    _endSession: function(nextState) {
      this.sdkDriver.disconnectSession();
      if (this._websocket) {
        this.stopListening(this._websocket);

        
        this._websocket.close();
        delete this._websocket;
      }

      navigator.mozLoop.calls.clearCallInProgress(this.get("windowId"));
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

  return ConversationStore;
})();
