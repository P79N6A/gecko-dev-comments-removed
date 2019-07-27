


"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "MozLoopService",
                                  "resource:///modules/loop/MozLoopService.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "LOOP_SESSION_TYPE",
                                  "resource:///modules/loop/MozLoopService.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "MozLoopPushHandler",
                                  "resource:///modules/loop/MozLoopPushHandler.jsm");

this.EXPORTED_SYMBOLS = ["LoopRooms", "roomsPushNotification"];

let gRoomsListFetched = false;
let gRooms = new Map();
let gCallbacks = new Map();

  






const roomsPushNotification = function(version, channelID) {
    return LoopRoomsInternal.onNotification(version, channelID);
  };

let LoopRoomsInternal = {
  getAll: function(callback) {
    Task.spawn(function*() {
      yield MozLoopService.register();

      if (gRoomsListFetched) {
        callback(null, [...gRooms.values()]);
        return;
      }
      
      let sessionType = MozLoopService.userProfile ? LOOP_SESSION_TYPE.FXA :
                        LOOP_SESSION_TYPE.GUEST;
      let rooms = yield this.requestRoomList(sessionType);
      
      
      for (let room of rooms) {
        let id = MozLoopService.generateLocalID();
        room.localRoomId = id;
        
        
        try {
          let details = yield this.requestRoomDetails(room.roomToken, sessionType);
          for (let attr in details) {
            room[attr] = details[attr]
          }
          delete room.currSize; 
          gRooms.set(id, room);
        }
        catch (error) {MozLoopService.log.warn(
          "failed GETing room details for roomToken = " + room.roomToken + ": ", error)}
      }
      callback(null, [...gRooms.values()]);
      return;
      }.bind(this)).catch((error) => {MozLoopService.log.error("getAll error:", error);
                                      callback(error)});
    return;
  },

  getRoomData: function(localRoomId, callback) {
    if (gRooms.has(localRoomId)) {
      callback(null, gRooms.get(localRoomId));
    } else {
      callback(new Error("Room data not found or not fetched yet for room with ID " + localRoomId));
    }
    return;
  },

  






  requestRoomList: function(sessionType) {
    return MozLoopService.hawkRequest(sessionType, "/rooms", "GET")
      .then(response => {
        let roomsList = JSON.parse(response.body);
        if (!Array.isArray(roomsList)) {
          
          
          throw new Error("Missing array of rooms in response.");
        }
        return roomsList;
      });
  },

  







  requestRoomDetails: function(token, sessionType) {
    return MozLoopService.hawkRequest(sessionType, "/rooms/" + token, "GET")
      .then(response => JSON.parse(response.body));
  },

  






  onNotification: function(version, channelID) {
    return;
  },

  createRoom: function(props, callback) {
    
    
    let localRoomId = MozLoopService.generateLocalID((id) => {gRooms.has(id)})
    let room = {localRoomId : localRoomId};
    for (let prop in props) {
      room[prop] = props[prop]
    }

    gRooms.set(localRoomId, room);
    this.addCallback(localRoomId, "RoomCreated", callback);
    MozLoopService.openChatWindow(null, "", "about:loopconversation#room/" + localRoomId);

    if (!"roomName" in props ||
        !"expiresIn" in props ||
        !"roomOwner" in props ||
        !"maxSize" in props) {
      this.postCallback(localRoomId, "RoomCreated",
                        new Error("missing required room create property"));
      return localRoomId;
    }

    let sessionType = MozLoopService.userProfile ? LOOP_SESSION_TYPE.FXA :
                                                   LOOP_SESSION_TYPE.GUEST;

    MozLoopService.hawkRequest(sessionType, "/rooms", "POST", props).then(
      (response) => {
        let data = JSON.parse(response.body);
        for (let attr in data) {
          room[attr] = data[attr]
        }
        delete room.expiresIn; 
        this.postCallback(localRoomId, "RoomCreated", null, room);
      },
      (error) => {
        this.postCallback(localRoomId, "RoomCreated", error);
      });

    return localRoomId;
  },

  



















  postCallback: function(localRoomId, callbackName, error, success) {
    let roomCallbacks = gCallbacks.get(localRoomId);
    if (!roomCallbacks) {
      
      
      
      gCallbacks.set(localRoomId, new Map([[
        callbackName,
        { callbackList: [], result: { error: error, success: success } }]]));
      return;
    }

    let namedCallback = roomCallbacks.get(callbackName);
    
    if (!namedCallback) {
      roomCallbacks.set(
        callbackName,
        {callbackList: [], result: {error: error, success: success}});
      return;
    }

    
    namedCallback.result = {error: error, success: success};

    
    namedCallback.callbackList.forEach((callback) => {
      callback(error, success);
    });
  },

  addCallback: function(localRoomId, callbackName, callback) {
    let roomCallbacks = gCallbacks.get(localRoomId);
    if (!roomCallbacks) {
      
      
      gCallbacks.set(localRoomId, new Map([[
        callbackName,
        {callbackList: [callback]}]]));
      return;
    }

    let namedCallback = roomCallbacks.get(callbackName);
    
    if (!namedCallback) {
      roomCallbacks.set(
        callbackName,
        {callbackList: [callback]});
      return;
    }

    
    if (namedCallback.callbackList.indexOf(callback) >= 0) {
      return;
    }
    namedCallback.callbackList.push(callback);

    
    
    let result = namedCallback.result;
    if (result) {
      callback(result.error, result.success);
    }
  },

  deleteCallback: function(localRoomId, callbackName, callback) {
    let roomCallbacks = gCallbacks.get(localRoomId);
    if (!roomCallbacks) {
      return;
    }

    let namedCallback = roomCallbacks.get(callbackName);
    if (!namedCallback) {
      return;
    }

    let i = namedCallback.callbackList.indexOf(callback);
    if (i >= 0) {
      namedCallback.callbackList.splice(i, 1);
    }

    return;
  },
};
Object.freeze(LoopRoomsInternal);








this.LoopRooms = {
  







  getAll: function(callback) {
    return LoopRoomsInternal.getAll(callback);
  },

  








  getRoomData: function(localRoomId, callback) {
    return LoopRoomsInternal.getRoomData(localRoomId, callback);
  },

  










  createRoom: function(roomProps, callback) {
    return LoopRoomsInternal.createRoom(roomProps, callback);
  },

  






  addCallback: function(localRoomId, callbackName, callback) {
    return LoopRoomsInternal.addCallback(localRoomId, callbackName, callback);
  },

  






  deleteCallback: function(localRoomId, callbackName, callback) {
    return LoopRoomsInternal.deleteCallback(localRoomId, callbackName, callback);
  },
};
Object.freeze(LoopRooms);
