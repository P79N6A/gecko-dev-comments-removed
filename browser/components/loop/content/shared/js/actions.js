





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
    


    GetWindowData: Action.define("getWindowData", {
      windowId: String
    }),

    


    ExtractTokenInfo: Action.define("extractTokenInfo", {
      windowPath: String
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

    



    FetchEmailLink: Action.define("fetchEmailLink", {
    }),

    


    CancelCall: Action.define("cancelCall", {
    }),

    


    RetryCall: Action.define("retryCall", {
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

    


    MediaConnected: Action.define("mediaConnected", {
    }),

    


    SetMute: Action.define("setMute", {
      
      type: String,
      
      enabled: Boolean
    }),

    



    CreateRoom: Action.define("createRoom", {
      
      
      nameTemplate: String,
      roomOwner: String
    }),

    



    CreateRoomError: Action.define("createRoomError", {
      error: Error
    }),

    



    DeleteRoom: Action.define("deleteRoom", {
      roomToken: String
    }),

    



    DeleteRoomError: Action.define("deleteRoomError", {
      error: Error
    }),

    



    GetAllRooms: Action.define("getAllRooms", {
    }),

    



    GetAllRoomsError: Action.define("getAllRoomsError", {
      error: Error
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

    



    CopyRoomUrl: Action.define("copyRoomUrl", {
      roomUrl: String
    }),

    



    EmailRoomUrl: Action.define("emailRoomUrl", {
      roomUrl: String
    }),

    


    RoomFailure: Action.define("roomFailure", {
      error: Object
    }),

    





    SetupRoomInfo: Action.define("setupRoomInfo", {
      roomName: String,
      roomOwner: String,
      roomToken: String,
      roomUrl: String
    }),

    





    UpdateRoomInfo: Action.define("updateRoomInfo", {
      roomName: String,
      roomOwner: String,
      roomUrl: String
    }),

    



    JoinRoom: Action.define("joinRoom", {
    }),

    





    JoinedRoom: Action.define("joinedRoom", {
      apiKey: String,
      sessionToken: String,
      sessionId: String,
      expires: Number
    }),

    


    LeaveRoom: Action.define("leaveRoom", {
    })
  };
})();
