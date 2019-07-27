





var loop = loop || {};
loop.store = loop.store || {};

loop.store.ActiveRoomStore = (function() {
  "use strict";

  var sharedActions = loop.shared.actions;
  var crypto = loop.crypto;
  var FAILURE_DETAILS = loop.shared.utils.FAILURE_DETAILS;
  var SCREEN_SHARE_STATES = loop.shared.utils.SCREEN_SHARE_STATES;

  
  
  var REST_ERRNOS = loop.shared.utils.REST_ERRNOS;

  var ROOM_STATES = loop.store.ROOM_STATES;

  var ROOM_INFO_FAILURES = loop.shared.utils.ROOM_INFO_FAILURES;

  var OPTIONAL_ROOMINFO_FIELDS = {
    urls: "roomContextUrls",
    description: "roomDescription",
    roomInfoFailure: "roomInfoFailure",
    roomName: "roomName"
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

      this._isDesktop = options.isDesktop || false;
    },

    


    getInitialStoreState: function() {
      return {
        roomState: ROOM_STATES.INIT,
        audioMuted: false,
        videoMuted: false,
        failureReason: undefined,
        
        
        
        
        used: false,
        localVideoDimensions: {},
        remoteVideoDimensions: {},
        screenSharingState: SCREEN_SHARE_STATES.INACTIVE,
        receivingScreenShare: false,
        
        roomContextUrls: null,
        
        roomCryptoKey: null,
        
        roomDescription: null,
        
        roomInfoFailure: null,
        
        roomName: null,
        
        socialShareButtonAvailable: false,
        socialShareProviders: null
      };
    },

    




    roomFailure: function(actionData) {
      function getReason(serverCode) {
        switch (serverCode) {
          case REST_ERRNOS.INVALID_TOKEN:
          case REST_ERRNOS.EXPIRED:
            return FAILURE_DETAILS.EXPIRED_OR_INVALID;
          default:
            return FAILURE_DETAILS.UNKNOWN;
        }
      }

      console.error("Error in state `" + this._storeState.roomState + "`:",
        actionData.error);

      this.setStoreState({
        error: actionData.error,
        failureReason: getReason(actionData.error.errno)
      });

      this._leaveRoom(actionData.error.errno === REST_ERRNOS.ROOM_FULL ?
          ROOM_STATES.FULL : ROOM_STATES.FAILED, actionData.failedJoinRequest);
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
        "screenSharingState",
        "receivingScreenShare",
        "remotePeerDisconnected",
        "remotePeerConnected",
        "windowUnload",
        "leaveRoom",
        "feedbackComplete",
        "videoDimensionsChanged",
        "startScreenShare",
        "endScreenShare",
        "updateSocialShareInfo",
        "connectionStatus"
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
            this.dispatchAction(new sharedActions.RoomFailure({
              error: error,
              failedJoinRequest: false
            }));
            return;
          }

          this.dispatchAction(new sharedActions.SetupRoomInfo({
            roomToken: actionData.roomToken,
            roomContextUrls: roomData.decryptedContext.urls,
            roomDescription: roomData.decryptedContext.description,
            roomName: roomData.decryptedContext.roomName,
            roomOwner: roomData.roomOwner,
            roomUrl: roomData.roomUrl,
            socialShareButtonAvailable: this._mozLoop.isSocialShareButtonAvailable(),
            socialShareProviders: this._mozLoop.getSocialShareProviders()
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
        roomCryptoKey: actionData.cryptoKey,
        roomState: ROOM_STATES.READY
      });

      this._mozLoop.rooms.on("update:" + actionData.roomToken,
        this._handleRoomUpdate.bind(this));
      this._mozLoop.rooms.on("delete:" + actionData.roomToken,
        this._handleRoomDelete.bind(this));

      this._getRoomDataForStandalone();
    },

    _getRoomDataForStandalone: function() {
      this._mozLoop.rooms.get(this._storeState.roomToken, function(err, result) {
        if (err) {
          
          
          console.error("Failed to get room data:", err);
          return;
        }

        var roomInfoData = new sharedActions.UpdateRoomInfo({
          roomOwner: result.roomOwner,
          roomUrl: result.roomUrl
        });

        if (!result.context && !result.roomName) {
          roomInfoData.roomInfoFailure = ROOM_INFO_FAILURES.NO_DATA;
          this.dispatcher.dispatch(roomInfoData);
          return;
        }

        
        if (result.roomName && !result.context) {
          roomInfoData.roomName = result.roomName;
          this.dispatcher.dispatch(roomInfoData);
          return;
        }

        if (!crypto.isSupported()) {
          roomInfoData.roomInfoFailure = ROOM_INFO_FAILURES.WEB_CRYPTO_UNSUPPORTED;
          this.dispatcher.dispatch(roomInfoData);
          return;
        }

        var roomCryptoKey = this.getStoreState("roomCryptoKey");

        if (!roomCryptoKey) {
          roomInfoData.roomInfoFailure = ROOM_INFO_FAILURES.NO_CRYPTO_KEY;
          this.dispatcher.dispatch(roomInfoData);
          return;
        }

        var dispatcher = this.dispatcher;

        crypto.decryptBytes(roomCryptoKey, result.context.value)
              .then(function(decryptedResult) {
          var realResult = JSON.parse(decryptedResult);

          roomInfoData.description = realResult.description;
          roomInfoData.urls = realResult.urls;
          roomInfoData.roomName = realResult.roomName;

          dispatcher.dispatch(roomInfoData);
        }, function(err) {
          roomInfoData.roomInfoFailure = ROOM_INFO_FAILURES.DECRYPT_FAILED;
          dispatcher.dispatch(roomInfoData);
        });
      }.bind(this));
    },

    





    setupRoomInfo: function(actionData) {
      if (this._onUpdateListener) {
        console.error("Room info already set up!");
        return;
      }

      this.setStoreState({
        roomContextUrls: actionData.roomContextUrls,
        roomDescription: actionData.roomDescription,
        roomName: actionData.roomName,
        roomOwner: actionData.roomOwner,
        roomState: ROOM_STATES.READY,
        roomToken: actionData.roomToken,
        roomUrl: actionData.roomUrl,
        socialShareButtonAvailable: actionData.socialShareButtonAvailable,
        socialShareProviders: actionData.socialShareProviders
      });

      this._onUpdateListener = this._handleRoomUpdate.bind(this);
      this._onDeleteListener = this._handleRoomDelete.bind(this);
      this._onSocialShareUpdate = this._handleSocialShareUpdate.bind(this);

      this._mozLoop.rooms.on("update:" + actionData.roomToken, this._onUpdateListener);
      this._mozLoop.rooms.on("delete:" + actionData.roomToken, this._onDeleteListener);
      window.addEventListener("LoopShareWidgetChanged", this._onSocialShareUpdate);
      window.addEventListener("LoopSocialProvidersChanged", this._onSocialShareUpdate);
    },

    




    updateRoomInfo: function(actionData) {
      var newState = {
        roomOwner: actionData.roomOwner,
        roomUrl: actionData.roomUrl
      };
      
      
      Object.keys(OPTIONAL_ROOMINFO_FIELDS).forEach(function(field) {
        if (actionData[field]) {
          newState[OPTIONAL_ROOMINFO_FIELDS[field]] = actionData[field];
        }
      });
      this.setStoreState(newState);
    },

    





    updateSocialShareInfo: function(actionData) {
      this.setStoreState({
        socialShareButtonAvailable: actionData.socialShareButtonAvailable,
        socialShareProviders: actionData.socialShareProviders
      });
    },

    





    _handleRoomUpdate: function(eventName, roomData) {
      this.dispatchAction(new sharedActions.UpdateRoomInfo({
        roomName: roomData.decryptedContext.roomName,
        roomOwner: roomData.roomOwner,
        roomUrl: roomData.roomUrl
      }));
    },

    





    _handleRoomDelete: function(eventName, roomData) {
      this._sdkDriver.forceDisconnectAll(function() {
        window.close();
      });
    },

    



    _handleSocialShareUpdate: function() {
      this.dispatchAction(new sharedActions.UpdateSocialShareInfo({
        socialShareButtonAvailable: this._mozLoop.isSocialShareButtonAvailable(),
        socialShareProviders: this._mozLoop.getSocialShareProviders()
      }));
    },

    


    joinRoom: function() {
      
      if (this.getStoreState().failureReason) {
        this.setStoreState({failureReason: undefined});
      }

      this.setStoreState({roomState: ROOM_STATES.MEDIA_WAIT});
    },

    



    gotMediaPermission: function() {
      this.setStoreState({roomState: ROOM_STATES.JOINING});

      this._mozLoop.rooms.join(this._storeState.roomToken,
        function(error, responseData) {
          if (error) {
            this.dispatchAction(new sharedActions.RoomFailure({
              error: error,
              
              
              
              
              
              failedJoinRequest: true
            }));
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

      
      actionData["sendTwoWayMediaTelemetry"] = this._isDesktop;

      this._sdkDriver.connectSession(actionData);

      this._mozLoop.addConversationContext(this._storeState.windowId,
                                           actionData.sessionId, "");
    },

    


    connectedToSdkServers: function() {
      this.setStoreState({
        roomState: ROOM_STATES.SESSION_CONNECTED
      });
    },

    




    connectionFailure: function(actionData) {
      




      if (this._isDesktop &&
          actionData.reason === FAILURE_DETAILS.UNABLE_TO_PUBLISH_MEDIA &&
          this.getStoreState().videoMuted === false) {
        
        
        this.setStoreState({videoMuted: true});
        this._sdkDriver.retryPublishWithoutVideo();
        return;
      }

      
      
      
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

    


    screenSharingState: function(actionData) {
      this.setStoreState({screenSharingState: actionData.state});

      this._mozLoop.setScreenShareState(
        this.getStoreState().windowId,
        actionData.state === SCREEN_SHARE_STATES.ACTIVE);
    },

    


    receivingScreenShare: function(actionData) {
      this.setStoreState({receivingScreenShare: actionData.receiving});
    },

    





    _handleSwitchBrowserShare: function(err, windowId) {
      if (err) {
        console.error("Error getting the windowId: " + err);
        this.dispatchAction(new sharedActions.ScreenSharingState({
          state: SCREEN_SHARE_STATES.INACTIVE
        }));
        return;
      }

      var screenSharingState = this.getStoreState().screenSharingState;

      if (screenSharingState === SCREEN_SHARE_STATES.INACTIVE) {
        
        var options = {
          videoSource: "browser",
          constraints: {
            browserWindow: windowId,
            scrollWithPage: true
          },
        };
        this._sdkDriver.startScreenShare(options);
      } else if (screenSharingState === SCREEN_SHARE_STATES.ACTIVE) {
        
        this._sdkDriver.switchAcquiredWindow(windowId);
      } else {
        console.error("Unexpectedly received windowId for browser sharing when pending");
      }
    },

    




    startScreenShare: function(actionData) {
      this.dispatchAction(new sharedActions.ScreenSharingState({
        state: SCREEN_SHARE_STATES.PENDING
      }));

      var options = {
        videoSource: actionData.type
      };
      if (options.videoSource === "browser") {
        this._browserSharingListener = this._handleSwitchBrowserShare.bind(this);

        
        
        
        this._mozLoop.addBrowserSharingListener(this._browserSharingListener);
      } else {
        this._sdkDriver.startScreenShare(options);
      }
    },

    


    endScreenShare: function() {
      if (this._browserSharingListener) {
        
        this._mozLoop.removeBrowserSharingListener(this._browserSharingListener);
        this._browserSharingListener = null;
      }

      if (this._sdkDriver.endScreenShare()) {
        this.dispatchAction(new sharedActions.ScreenSharingState({
          state: SCREEN_SHARE_STATES.INACTIVE
        }));
      }
    },

    


    remotePeerConnected: function() {
      this.setStoreState({
        roomState: ROOM_STATES.HAS_PARTICIPANTS,
        used: true
      });

      
      this._mozLoop.setLoopPref("seenToS", "seen");
    },

    




    remotePeerDisconnected: function() {
      this.setStoreState({roomState: ROOM_STATES.SESSION_CONNECTED});
    },

    




    connectionStatus: function(actionData) {
      this._mozLoop.rooms.sendConnectionStatus(this.getStoreState("roomToken"),
        this.getStoreState("sessionToken"),
        actionData);
    },

    


    windowUnload: function() {
      this._leaveRoom(ROOM_STATES.CLOSING);

      if (!this._onUpdateListener) {
        return;
      }

      
      var roomToken = this.getStoreState().roomToken;
      this._mozLoop.rooms.off("update:" + roomToken, this._onUpdateListener);
      this._mozLoop.rooms.off("delete:" + roomToken, this._onDeleteListener);
      window.removeEventListener("LoopShareWidgetChanged", this._onShareWidgetUpdate);
      window.removeEventListener("LoopSocialProvidersChanged", this._onSocialProvidersUpdate);
      delete this._onUpdateListener;
      delete this._onDeleteListener;
      delete this._onShareWidgetUpdate;
      delete this._onSocialProvidersUpdate;
    },

    


    leaveRoom: function() {
      this._leaveRoom(ROOM_STATES.ENDED);
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
            this.dispatchAction(new sharedActions.RoomFailure({
              error: error,
              failedJoinRequest: false
            }));
            return;
          }

          this._setRefreshTimeout(responseData.expires);
        }.bind(this));
    },

    








    _leaveRoom: function(nextState, failedJoinRequest) {
      if (loop.standaloneMedia) {
        loop.standaloneMedia.multiplexGum.reset();
      }

      this._mozLoop.setScreenShareState(
        this.getStoreState().windowId,
        false);

      if (this._browserSharingListener) {
        
        this._mozLoop.removeBrowserSharingListener(this._browserSharingListener);
        this._browserSharingListener = null;
      }

      
      this._sdkDriver.disconnectSession();

      if (this._timeout) {
        clearTimeout(this._timeout);
        delete this._timeout;
      }

      if (!failedJoinRequest &&
          (this._storeState.roomState === ROOM_STATES.JOINING ||
           this._storeState.roomState === ROOM_STATES.JOINED ||
           this._storeState.roomState === ROOM_STATES.SESSION_CONNECTED ||
           this._storeState.roomState === ROOM_STATES.HAS_PARTICIPANTS)) {
        this._mozLoop.rooms.leave(this._storeState.roomToken,
          this._storeState.sessionToken);
      }

      this.setStoreState({roomState: nextState});
    },

    


    feedbackComplete: function() {
      
      
      this.setStoreState(this.getInitialStoreState());
    },

    





    videoDimensionsChanged: function(actionData) {
      
      
      
      var storeProp = (actionData.isLocal ? "local" : "remote") + "VideoDimensions";
      var nextState = {};
      nextState[storeProp] = this.getStoreState()[storeProp];
      nextState[storeProp][actionData.videoType] = actionData.dimensions;
      this.setStoreState(nextState);
    }
  });

  return ActiveRoomStore;
})();
