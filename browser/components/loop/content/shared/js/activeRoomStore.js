





var loop = loop || {};
loop.store = loop.store || {};
loop.store.ActiveRoomStore = (function() {
  "use strict";

  var sharedActions = loop.shared.actions;

  var ROOM_STATES = loop.store.ROOM_STATES = {
    
    INIT: "room-init",
    
    GATHER: "room-gather",
    
    READY: "room-ready",
    
    JOINED: "room-joined",
    
    SESSION_CONNECTED: "room-session-connected",
    
    HAS_PARTICIPANTS: "room-has-participants",
    
    FAILED: "room-failed",
    
    FULL: "room-full"
  };

  











  function ActiveRoomStore(options) {
    options = options || {};

    if (!options.dispatcher) {
      throw new Error("Missing option dispatcher");
    }
    this._dispatcher = options.dispatcher;

    if (!options.mozLoop) {
      throw new Error("Missing option mozLoop");
    }
    this._mozLoop = options.mozLoop;

    if (!options.sdkDriver) {
      throw new Error("Missing option sdkDriver");
    }
    this._sdkDriver = options.sdkDriver;

    
    
    
    
    this._dispatcher.register(this, [
      "setupWindowData",
      "fetchServerData"
    ]);

    











    this._storeState = {
      roomState: ROOM_STATES.INIT,
      audioMuted: false,
      videoMuted: false
    };
  }

  ActiveRoomStore.prototype = _.extend({
    



    expiresTimeFactor: 0.9,

    getStoreState: function() {
      return this._storeState;
    },

    setStoreState: function(newState) {
      for (var key in newState) {
        this._storeState[key] = newState[key];
      }
      this.trigger("change");
    },

    




    roomFailure: function(actionData) {
      console.error("Error in state `" + this._storeState.roomState + "`:",
        actionData.error);

      this.setStoreState({
        error: actionData.error,
        roomState: actionData.error.errno === 202 ? ROOM_STATES.FULL
                                                  : ROOM_STATES.FAILED
      });
    },

    



    _registerActions: function() {
      this._dispatcher.register(this, [
        "roomFailure",
        "updateRoomInfo",
        "joinRoom",
        "joinedRoom",
        "connectedToSdkServers",
        "connectionFailure",
        "setMute",
        "remotePeerDisconnected",
        "remotePeerConnected",
        "windowUnload",
        "leaveRoom"
      ]);
    },

    







    setupWindowData: function(actionData) {
      if (actionData.type !== "room") {
        
        return;
      }

      this._registerActions();

      this.setStoreState({
        roomState: ROOM_STATES.GATHER
      });

      
      this._mozLoop.rooms.get(actionData.roomToken,
        function(error, roomData) {
          if (error) {
            this._dispatcher.dispatch(new sharedActions.RoomFailure({
              error: error
            }));
            return;
          }

          this._dispatcher.dispatch(
            new sharedActions.UpdateRoomInfo({
            roomToken: actionData.roomToken,
            roomName: roomData.roomName,
            roomOwner: roomData.roomOwner,
            roomUrl: roomData.roomUrl
          }));

          
          
          this._dispatcher.dispatch(new sharedActions.JoinRoom());
        }.bind(this));
    },

    







    fetchServerData: function(actionData) {
      if (actionData.windowType !== "room") {
        
        return;
      }

      this._registerActions();

      this.setStoreState({
        roomToken: actionData.token,
        roomState: ROOM_STATES.READY
      });
    },

    





    updateRoomInfo: function(actionData) {
      this.setStoreState({
        roomName: actionData.roomName,
        roomOwner: actionData.roomOwner,
        roomState: ROOM_STATES.READY,
        roomToken: actionData.roomToken,
        roomUrl: actionData.roomUrl
      });
    },

    


    joinRoom: function() {
      this._mozLoop.rooms.join(this._storeState.roomToken,
        function(error, responseData) {
          if (error) {
            this._dispatcher.dispatch(
              new sharedActions.RoomFailure({error: error}));
            return;
          }

          this._dispatcher.dispatch(new sharedActions.JoinedRoom({
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
    },

    


    connectedToSdkServers: function() {
      this.setStoreState({
        roomState: ROOM_STATES.SESSION_CONNECTED
      });
    },

    


    connectionFailure: function() {
      
      
      
      this._leaveRoom(ROOM_STATES.FAILED);
    },

    




    setMute: function(actionData) {
      var muteState = {};
      muteState[actionData.type + "Muted"] = !actionData.enabled;
      this.setStoreState(muteState);
    },

    


    remotePeerConnected: function() {
      this.setStoreState({
        roomState: ROOM_STATES.HAS_PARTICIPANTS
      });
    },

    


    remotePeerDisconnected: function() {
      
      
      this.setStoreState({
        roomState: ROOM_STATES.SESSION_CONNECTED
      });
    },

    


    windowUnload: function() {
      this._leaveRoom();
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
            this._dispatcher.dispatch(
              new sharedActions.RoomFailure({error: error}));
            return;
          }

          this._setRefreshTimeout(responseData.expires);
        }.bind(this));
    },

    






    _leaveRoom: function(nextState) {
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

      this.setStoreState({
        roomState: nextState ? nextState : ROOM_STATES.READY
      });
    }

  }, Backbone.Events);

  return ActiveRoomStore;

})();
