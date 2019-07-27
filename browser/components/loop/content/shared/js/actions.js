





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

    


    PeerHungupCall: Action.define("peerHungupCall", {
    }),

    




    ConnectionProgress: Action.define("connectionProgress", {
      
      wsState: String
    }),

    


    ConnectionFailure: Action.define("connectionFailure", {
      
      reason: String
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

    



    CopyRoomUrl: Action.define("copyRoomUrl", {
      roomUrl: String
    })
  };
})();
