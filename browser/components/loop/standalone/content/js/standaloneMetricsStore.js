



var loop = loop || {};
loop.store = loop.store || {};










loop.store.StandaloneMetricsStore = (function() {
  "use strict";

  var ROOM_STATES = loop.store.ROOM_STATES;
  var FAILURE_DETAILS = loop.shared.utils.FAILURE_DETAILS;

  loop.store.metrics = loop.store.metrics || {};

  var METRICS_GA_CATEGORY = loop.store.METRICS_GA_CATEGORY = {
    general: "/conversation/ interactions",
    download: "Firefox Downloads"
  };

  var METRICS_GA_ACTIONS = loop.store.METRICS_GA_ACTIONS = {
    audioMute: "audio mute",
    button: "button click",
    download: "download button click",
    faceMute: "face mute",
    failed: "failed",
    linkClick: "link click",
    pageLoad: "page load messages",
    success: "success",
    support: "support link click"
  };

  var StandaloneMetricsStore = loop.store.createStore({
    actions: [
      "connectedToSdkServers",
      "connectionFailure",
      "gotMediaPermission",
      "joinRoom",
      "joinedRoom",
      "leaveRoom",
      "mediaConnected",
      "recordClick",
      "remotePeerConnected",
      "retryAfterRoomFailure"
    ],

    





    initialize: function(options) {
      options = options || {};

      if (!options.activeRoomStore) {
        throw new Error("Missing option activeRoomStore");
      }

      
      
      
      if (window.ga) {
        this.activeRoomStore = options.activeRoomStore;

        this.listenTo(options.activeRoomStore, "change",
          this._onActiveRoomStoreChange.bind(this));
      }
    },

    






    getInitialStoreState: function() {
      return {
        audioMuted: false,
        roomState: ROOM_STATES.INIT,
        videoMuted: false
      };
    },

    






    _storeEvent: function(category, action, label) {
      
      if (!window.ga) {
        return;
      }

      
      window.ga("send", "event", category, action, label);
    },

    


    connectedToSdkServers: function() {
      this._storeEvent(METRICS_GA_CATEGORY.general, METRICS_GA_ACTIONS.success,
        "Joined sdk servers");
    },

    




    connectionFailure: function(actionData) {
      if (actionData.reason === FAILURE_DETAILS.MEDIA_DENIED) {
        this._storeEvent(METRICS_GA_CATEGORY.general, METRICS_GA_ACTIONS.failed,
          "Media denied");
      } else if (actionData.reason === FAILURE_DETAILS.NO_MEDIA) {
        this._storeEvent(METRICS_GA_CATEGORY.general, METRICS_GA_ACTIONS.failed,
          "No media");
      }
    },

    


    gotMediaPermission: function() {
      this._storeEvent(METRICS_GA_CATEGORY.general, METRICS_GA_ACTIONS.success,
        "Media granted");
    },

    


    joinRoom: function() {
      this._storeEvent(METRICS_GA_CATEGORY.general, METRICS_GA_ACTIONS.button,
        "Join the conversation");
    },

    


    joinedRoom: function() {
      this._storeEvent(METRICS_GA_CATEGORY.general, METRICS_GA_ACTIONS.success,
        "Joined room");
    },

    


    leaveRoom: function() {
      this._storeEvent(METRICS_GA_CATEGORY.general, METRICS_GA_ACTIONS.button,
        "Leave conversation");
    },

    


    mediaConnected: function() {
      this._storeEvent(METRICS_GA_CATEGORY.general, METRICS_GA_ACTIONS.success,
        "Media connected");
    },

    





    recordClick: function(actionData) {
      this._storeEvent(METRICS_GA_CATEGORY.general, METRICS_GA_ACTIONS.linkClick,
        actionData.linkInfo);
    },

    


    remotePeerConnected: function() {
      this._storeEvent(METRICS_GA_CATEGORY.general, METRICS_GA_ACTIONS.success,
        "Remote peer connected");
    },

    



    retryAfterRoomFailure: function() {
      this._storeEvent(METRICS_GA_CATEGORY.general, METRICS_GA_ACTIONS.button,
        "Retry failed room");
    },

    



    _onActiveRoomStoreChange: function() {
      var roomStore = this.activeRoomStore.getStoreState();

      this._checkRoomState(roomStore.roomState, roomStore.failureReason);

      this._checkMuteState("audio", roomStore.audioMuted);
      this._checkMuteState("video", roomStore.videoMuted);
    },

    







    _checkRoomState: function(roomState, failureReason) {
      if (this._storeState.roomState === roomState) {
        return;
      }
      this._storeState.roomState = roomState;

      if (roomState === ROOM_STATES.FAILED &&
          failureReason === FAILURE_DETAILS.EXPIRED_OR_INVALID) {
        this._storeEvent(METRICS_GA_CATEGORY.general, METRICS_GA_ACTIONS.pageLoad,
          "Link expired or invalid");
      }

      if (roomState === ROOM_STATES.FULL) {
        this._storeEvent(METRICS_GA_CATEGORY.general, METRICS_GA_ACTIONS.pageLoad,
          "Room full");
      }
    },

    






    _checkMuteState: function(type, muted) {
      var muteItem = type + "Muted";
      if (this._storeState[muteItem] === muted) {
        return;
      }
      this._storeState[muteItem] = muted;

      var muteType = type === "audio" ? METRICS_GA_ACTIONS.audioMute : METRICS_GA_ACTIONS.faceMute;
      var muteState = muted ? "mute" : "unmute";

      this._storeEvent(METRICS_GA_CATEGORY.general, muteType, muteState);
    }
  });

  return StandaloneMetricsStore;
})();
