


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


XPCOMUtils.defineLazyGetter(this, "log", () => {
  let ConsoleAPI = Cu.import("resource://gre/modules/devtools/Console.jsm", {}).ConsoleAPI;
  let consoleOptions = {
    maxLogLevel: Services.prefs.getCharPref(PREF_LOG_LEVEL).toLowerCase(),
    prefix: "Loop",
  };
  return new ConsoleAPI(consoleOptions);
});

this.EXPORTED_SYMBOLS = ["LoopRooms", "roomsPushNotification"];

let gRoomsListFetched = false;
let gRooms = new Map();

  






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
        room.localRoomID = id;
        
        
        try {
          let details = yield this.requestRoomDetails(room.roomToken, sessionType);
          for (let attr in details) {
            room[attr] = details[attr]
          }
          gRooms.set(id, room);
        }
        catch (error) {log.warn("failed GETing room details for roomToken = " + room.roomToken + ": ", error)}
      }
      callback(null, [...gRooms.values()]);
      return;
      }.bind(this)).catch((error) => {log.error("getAll error:", error);
                                      callback(error)});
    return;
  },

  getRoomData: function(roomID, callback) {
    if (gRooms.has(roomID)) {
      callback(null, gRooms.get(roomID));
    } else {
      callback(new Error("Room data not found or not fetched yet for room with ID " + roomID));
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
};
Object.freeze(LoopRoomsInternal);








this.LoopRooms = {
  







  getAll: function(callback) {
    return LoopRoomsInternal.getAll(callback);
  },

  








  getRoomData: function(roomID, callback) {
    return LoopRoomsInternal.getRoomData(roomID, callback);
  },
};
Object.freeze(LoopRooms);

