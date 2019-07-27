





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
    



    FetchEmailLink: Action.define("fetchEmailLink", {
    }),

    


    GatherCallData: Action.define("gatherCallData", {
      
      windowId: [String, null],
      outgoing: Boolean
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

    



    GetAllRooms: Action.define("getAllRooms", {
    }),

    



    GetAllRoomsError: Action.define("getAllRoomsError", {
      error: String
    }),

    



    UpdateRoomList: Action.define("updateRoomList", {
      roomList: Array
    }),

    





    SetupEmptyRoom: Action.define("setupEmptyRoom", {
      localRoomId: String
    }),
  };
})();
