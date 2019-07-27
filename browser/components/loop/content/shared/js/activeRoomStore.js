





var loop = loop || {};
loop.store = loop.store || {};

loop.store.ActiveRoomStore = (function() {
  "use strict";

  var sharedActions = loop.shared.actions;
  var FAILURE_REASONS = loop.shared.utils.FAILURE_REASONS;

  
  
  var SERVER_CODES = loop.store.SERVER_CODES = {
    INVALID_TOKEN: 105,
    EXPIRED: 111,
    ROOM_FULL: 202
  };

  var ROOM_STATES = loop.store.ROOM_STATES = {
    
    INIT: "room-init",
    
    GATHER: "room-gather",
    
    READY: "room-ready",
    
    MEDIA_WAIT: "room-media-wait",
    
    JOINED: "room-joined",
    
    SESSION_CONNECTED: "room-session-connected",
    
    HAS_PARTICIPANTS: "room-has-participants",
    
    FAILED: "room-failed",
    
    FULL: "room-full",
    
    ENDED: "room-ended",
    
    CLOSING: "room-closing"
  };

  








  var ActiveRoomStore = loop.store.createStore({
    



    expiresTimeFactor: 0.9,

    
    
    
    
    actions: [
      "setupWindowData",
      "fetchServerData"
    ],

    initialize: function(options) {
      if (!options.mozLoop) {
        throw new Error("Missing option mozLoop");
      }
      this._mozLoop = options.mozLoop;

      if (!options.sdkDriver) {
        throw new Error("Missing option sdkDriver");
      }
      this._sdkDriver = options.sdkDriver;
    },

    


    getInitialStoreState: function() {
      return {
        roomState: ROOM_STATES.INIT,
        audioMuted: false,
        videoMuted: false,
        failureReason: undefined
      };
    },

    




    roomFailure: function(actionData) {
      function getReason(serverCode) {
        switch (serverCode) {
          case SERVER_CODES.INVALID_TOKEN:
          case SERVER_CODES.EXPIRED:
            return FAILURE_REASONS.EXPIRED_OR_INVALID;
          default:
            return FAILURE_REASONS.UNKNOWN;
        }
      }

      console.error("Error in state `" + this._storeState.roomState + "`:",
        actionData.error);

      this.setStoreState({
        error: actionData.error,
        failureReason: getReason(actionData.error.errno)
      });

      this._leaveRoom(actionData.error.errno === SERVER_CODES.ROOM_FULL ?
          ROOM_STATES.FULL : ROOM_STATES.FAILED);
    },

    



    _registerPostSetupActions: function() {
      this.dispatcher.register(this, [
        "roomFailure",
        "setupRoomInfo",
        "updateRoomInfo",
        "gotMediaPermission",
        "joinRoom",
        "joinedRoom",
        "connectedToSdkServers",
        "connectionFailure",
        "setMute",
        "remotePeerDisconnected",
        "remotePeerConnected",
        "windowUnload",
        "leaveRoom",
        "feedbackComplete"
      ]);
    },

    







    setupWindowData: function(actionData) {
      if (actionData.type !== "room") {
        
        return;
      }

      this._registerPostSetupActions();

      this.setStoreState({
        roomState: ROOM_STATES.GATHER,
        windowId: actionData.windowId
      });

      
      this._mozLoop.rooms.get(actionData.roomToken,
        function(error, roomData) {
          if (error) {
            this.dispatchAction(new sharedActions.RoomFailure({error: error}));
            return;
          }

          this.dispatchAction(new sharedActions.SetupRoomInfo({
            roomToken: actionData.roomToken,
            roomName: roomData.roomName,
            roomOwner: roomData.roomOwner,
            roomUrl: roomData.roomUrl
          }));

          
          
          this.dispatchAction(new sharedActions.JoinRoom());
        }.bind(this));
    },

    







    fetchServerData: function(actionData) {
      if (actionData.windowType !== "room") {
        
        return;
      }

      this._registerPostSetupActions();

      this.setStoreState({
        roomToken: actionData.token,
        roomState: ROOM_STATES.READY
      });

      this._mozLoop.rooms.on("update:" + actionData.roomToken,
        this._handleRoomUpdate.bind(this));
      this._mozLoop.rooms.on("delete:" + actionData.roomToken,
        this._handleRoomDelete.bind(this));
    },

    





    setupRoomInfo: function(actionData) {
      this.setStoreState({
        roomName: actionData.roomName,
        roomOwner: actionData.roomOwner,
        roomState: ROOM_STATES.READY,
        roomToken: actionData.roomToken,
        roomUrl: actionData.roomUrl
      });

      this._mozLoop.rooms.on("update:" + actionData.roomToken,
        this._handleRoomUpdate.bind(this));
      this._mozLoop.rooms.on("delete:" + actionData.roomToken,
        this._handleRoomDelete.bind(this));
    },

    




    updateRoomInfo: function(actionData) {
      this.setStoreState({
        roomName: actionData.roomName,
        roomOwner: actionData.roomOwner,
        roomUrl: actionData.roomUrl
      });
    },

    





    _handleRoomUpdate: function(eventName, roomData) {
      this.dispatchAction(new sharedActions.UpdateRoomInfo({
        roomName: roomData.roomName,
        roomOwner: roomData.roomOwner,
        roomUrl: roomData.roomUrl
      }));
    },

    





    _handleRoomDelete: function(eventName, roomData) {
      this._sdkDriver.forceDisconnectAll(function() {
        window.close();
      });
    },

    


    joinRoom: function() {
      
      if (this.getStoreState().failureReason) {
        this.setStoreState({failureReason: undefined});
      }

      this.setStoreState({roomState: ROOM_STATES.MEDIA_WAIT});
    },

    



    gotMediaPermission: function() {
      this._mozLoop.rooms.join(this._storeState.roomToken,
        function(error, responseData) {
          if (error) {
            this.dispatchAction(new sharedActions.RoomFailure({error: error}));
            return;
          }

          this.dispatchAction(new sharedActions.JoinedRoom({
            apiKey: responseData.apiKey,
            sessionToken: responseData.sessionToken,
            sessionId: responseData.sessionId,
            expires: responseData.expires
          }));
        }.bind(this));
    },

    






    joinedRoom: function(actionData) {
      this.setStoreState({
        apiKey: actionData.apiKey,
        sessionToken: actionData.sessionToken,
        sessionId: actionData.sessionId,
        roomState: ROOM_STATES.JOINED
      });

      this._setRefreshTimeout(actionData.expires);
      this._sdkDriver.connectSession(actionData);

      this._mozLoop.addConversationContext(this._storeState.windowId,
                                           actionData.sessionId, "");

      
      
      
      if (!this._storeState.roomName) {
        this._mozLoop.rooms.get(this._storeState.roomToken,
          function(err, result) {
            if (err) {
              console.error("Failed to get room data:", err);
              return;
            }

            this.dispatcher.dispatch(new sharedActions.UpdateRoomInfo(result));
        }.bind(this));
      }
    },

    


    connectedToSdkServers: function() {
      this.setStoreState({
        roomState: ROOM_STATES.SESSION_CONNECTED
      });
    },

    




    connectionFailure: function(actionData) {
      
      
      
      this.setStoreState({
        failureReason: actionData.reason
      });

      this._leaveRoom(ROOM_STATES.FAILED);
    },

    




    setMute: function(actionData) {
      var muteState = {};
      muteState[actionData.type + "Muted"] = !actionData.enabled;
      this.setStoreState(muteState);
    },

    


    remotePeerConnected: function() {
      this.setStoreState({roomState: ROOM_STATES.HAS_PARTICIPANTS});

      
      this._mozLoop.setLoopPref("seenToS", "seen");
    },

    




    remotePeerDisconnected: function() {
      this.setStoreState({roomState: ROOM_STATES.SESSION_CONNECTED});
    },

    


    windowUnload: function() {
      this._leaveRoom(ROOM_STATES.CLOSING);

      
      var roomToken = this.getStoreState().roomToken;
      this._mozLoop.rooms.off("update:" + roomToken);
      this._mozLoop.rooms.off("delete:" + roomToken);
    },

    


    leaveRoom: function() {
      this._leaveRoom();
    },

    




    _setRefreshTimeout: function(expireTime) {
      this._timeout = setTimeout(this._refreshMembership.bind(this),
        expireTime * this.expiresTimeFactor * 1000);
    },

    



    _refreshMembership: function() {
      this._mozLoop.rooms.refreshMembership(this._storeState.roomToken,
        this._storeState.sessionToken,
        function(error, responseData) {
          if (error) {
            this.dispatchAction(new sharedActions.RoomFailure({error: error}));
            return;
          }

          this._setRefreshTimeout(responseData.expires);
        }.bind(this));
    },

    






    _leaveRoom: function(nextState) {
      if (loop.standaloneMedia) {
        loop.standaloneMedia.multiplexGum.reset();
      }

      this._sdkDriver.disconnectSession();

      if (this._timeout) {
        clearTimeout(this._timeout);
        delete this._timeout;
      }

      if (this._storeState.roomState === ROOM_STATES.JOINED ||
          this._storeState.roomState === ROOM_STATES.SESSION_CONNECTED ||
          this._storeState.roomState === ROOM_STATES.HAS_PARTICIPANTS) {
        this._mozLoop.rooms.leave(this._storeState.roomToken,
          this._storeState.sessionToken);
      }

      this.setStoreState({roomState: nextState || ROOM_STATES.ENDED});
    },

    


    feedbackComplete: function() {
      
      
      this.setStoreState(this.getInitialStoreState());
    }
  });

  return ActiveRoomStore;
})();
