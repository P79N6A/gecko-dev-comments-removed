





var loop = loop || {};
loop.shared = loop.shared || {};
loop.shared.actions = (function() {
  "use strict";

  






  function Action(name, schema, values) {
    var validatedData = new loop.validate.Validator(schema || {})
                                         .validate(values || {});
    for (var prop in validatedData)
      this[prop] = validatedData[prop];

    this.name = name;
  }

  Action.define = function(name, schema) {
    return Action.bind(null, name, schema);
  };

  return {
    Action: Action,

    


    GetWindowData: Action.define("getWindowData", {
      windowId: String
    }),

    


    ExtractTokenInfo: Action.define("extractTokenInfo", {
      windowPath: String,
      windowHash: String
    }),

    



    SetupWindowData: Action.define("setupWindowData", {
      windowId: String,
      type: String

      
      
      
      
    }),

    



    FetchServerData: Action.define("fetchServerData", {
      
      token: String,
      windowType: String
    }),

    


    WindowUnload: Action.define("windowUnload", {
    }),

    



    FetchRoomEmailLink: Action.define("fetchRoomEmailLink", {
      roomOwner: String,
      roomName: String
    }),

    


    CancelCall: Action.define("cancelCall", {
    }),

    


    RetryCall: Action.define("retryCall", {
    }),

    


    AcceptCall: Action.define("acceptCall", {
      callType: String
    }),

    


    DeclineCall: Action.define("declineCall", {
      blockCaller: Boolean
    }),

    



    ConnectCall: Action.define("connectCall", {
      
      
      sessionData: Object
    }),

    


    HangupCall: Action.define("hangupCall", {
    }),

    




    RemotePeerDisconnected: Action.define("remotePeerDisconnected", {
      peerHungup: Boolean
    }),

    




    ConnectionProgress: Action.define("connectionProgress", {
      
      wsState: String
    }),

    


    ConnectionFailure: Action.define("connectionFailure", {
      
      reason: String
    }),

    


    ConnectedToSdkServers: Action.define("connectedToSdkServers", {
    }),

    


    RemotePeerConnected: Action.define("remotePeerConnected", {
    }),

    



    SetupStreamElements: Action.define("setupStreamElements", {
      
      publisherConfig: Object,
      
      getLocalElementFunc: Function,
      
      
      
      
      getRemoteElementFunc: Function
    }),

    


    GotMediaPermission: Action.define("gotMediaPermission", {
    }),

    


    MediaConnected: Action.define("mediaConnected", {
    }),

    



    VideoDimensionsChanged: Action.define("videoDimensionsChanged", {
      videoType: String,
      dimensions: Object
    }),

    


    SetMute: Action.define("setMute", {
      
      type: String,
      
      enabled: Boolean
    }),

    


    StartScreenShare: Action.define("startScreenShare", {
      
      type: String
    }),

    


    EndScreenShare: Action.define("endScreenShare", {
    }),

    


    ScreenSharingState: Action.define("screenSharingState", {
      
      state: String
    }),

    


    ReceivingScreenShare: Action.define("receivingScreenShare", {
      receiving: Boolean
    }),

    



    CreateRoom: Action.define("createRoom", {
      
      
      nameTemplate: String,
      roomOwner: String,
      
      
    }),

    



    CreatedRoom: Action.define("createdRoom", {
      roomToken: String
    }),

    



    CreateRoomError: Action.define("createRoomError", {
      
      
      
      error: Object
    }),

    



    DeleteRoom: Action.define("deleteRoom", {
      roomToken: String
    }),

    



    DeleteRoomError: Action.define("deleteRoomError", {
      
      
      
      error: Object
    }),

    



    GetAllRooms: Action.define("getAllRooms", {
    }),

    



    GetAllRoomsError: Action.define("getAllRoomsError", {
      
      
      
      error: [Error, Object]
    }),

    



    UpdateRoomList: Action.define("updateRoomList", {
      roomList: Array
    }),

    



    OpenRoom: Action.define("openRoom", {
      roomToken: String
    }),

    



    RenameRoom: Action.define("renameRoom", {
      roomToken: String,
      newRoomName: String
    }),

    



    RenameRoomError: Action.define("renameRoomError", {
      error: [Error, Object]
    }),

    



    CopyRoomUrl: Action.define("copyRoomUrl", {
      roomUrl: String
    }),

    



    EmailRoomUrl: Action.define("emailRoomUrl", {
      roomUrl: String
    }),

    



    ShareRoomUrl: Action.define("shareRoomUrl", {
      provider: Object,
      roomUrl: String
    }),

    



    AddSocialShareButton: Action.define("addSocialShareButton", {
    }),

    



    AddSocialShareProvider: Action.define("addSocialShareProvider", {
    }),

    


    RoomFailure: Action.define("roomFailure", {
      error: Object,
      
      failedJoinRequest: Boolean
    }),

    





    SetupRoomInfo: Action.define("setupRoomInfo", {
      
      roomOwner: String,
      roomToken: String,
      roomUrl: String,
      socialShareButtonAvailable: Boolean,
      socialShareProviders: Array
    }),

    





    UpdateRoomInfo: Action.define("updateRoomInfo", {
      
      
      roomOwner: String,
      roomUrl: String
      
      
    }),

    



    UpdateSocialShareInfo: Action.define("updateSocialShareInfo", {
      socialShareButtonAvailable: Boolean,
      socialShareProviders: Array
    }),

    



    JoinRoom: Action.define("joinRoom", {
    }),

    





    JoinedRoom: Action.define("joinedRoom", {
      apiKey: String,
      sessionToken: String,
      sessionId: String,
      expires: Number
    }),

    



    FeedbackComplete: Action.define("feedbackComplete", {
    }),

    


    LeaveRoom: Action.define("leaveRoom", {
    }),

    


    RequireFeedbackDetails: Action.define("requireFeedbackDetails", {
    }),

    


    SendFeedback: Action.define("sendFeedback", {
      happy: Boolean,
      category: String,
      description: String
    }),

    


    SendFeedbackError: Action.define("sendFeedbackError", {
      error: Error
    })
  };
})();
