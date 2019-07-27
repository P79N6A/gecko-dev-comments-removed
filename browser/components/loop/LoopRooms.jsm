


"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
const {MozLoopService, LOOP_SESSION_TYPE} = Cu.import("resource:///modules/loop/MozLoopService.jsm", {});
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyGetter(this, "eventEmitter", function() {
  const {EventEmitter} = Cu.import("resource://gre/modules/devtools/event-emitter.js", {});
  return new EventEmitter();
});

this.EXPORTED_SYMBOLS = ["LoopRooms", "roomsPushNotification"];

const roomsPushNotification = function(version, channelID) {
  return LoopRoomsInternal.onNotification(version, channelID);
};





let gDirty = true;







const extend = function(target, source) {
  for (let key of Object.getOwnPropertyNames(source)) {
    target[key] = source[key];
  }
  return target;
};








let LoopRoomsInternal = {
  rooms: new Map(),

  









  getAll: function(version = null, callback) {
    if (!callback) {
      callback = version;
      version = null;
    }

    Task.spawn(function* () {
      yield MozLoopService.register();

      if (!gDirty) {
        callback(null, [...this.rooms.values()]);
        return;
      }

      
      let sessionType = MozLoopService.userProfile ? LOOP_SESSION_TYPE.FXA :
                        LOOP_SESSION_TYPE.GUEST;
      let url = "/rooms" + (version ? "?version=" + encodeURIComponent(version) : "");
      let response = yield MozLoopService.hawkRequest(sessionType, url, "GET");
      let roomsList = JSON.parse(response.body);
      if (!Array.isArray(roomsList)) {
        throw new Error("Missing array of rooms in response.");
      }

      
      
      for (let room of roomsList) {
        this.rooms.set(room.roomToken, room);
        yield LoopRooms.promise("get", room.roomToken);
      }

      
      gDirty = false;
      callback(null, [...this.rooms.values()]);
    }.bind(this)).catch(error => {
      callback(error);
    });
  },

  









  get: function(roomToken, callback) {
    let room = this.rooms.has(roomToken) ? this.rooms.get(roomToken) : {};
    
    if (!room || gDirty || !("participants" in room)) {
      let sessionType = MozLoopService.userProfile ? LOOP_SESSION_TYPE.FXA :
                        LOOP_SESSION_TYPE.GUEST;
      MozLoopService.hawkRequest(sessionType, "/rooms/" + encodeURIComponent(roomToken), "GET")
        .then(response => {
          let eventName = ("roomToken" in room) ? "add" : "update";
          extend(room, JSON.parse(response.body));
          
          if ("currSize" in room) {
            delete room.currSize;
          }
          this.rooms.set(roomToken, room);

          eventEmitter.emit(eventName, room);
          callback(null, room);
        }, err => callback(err)).catch(err => callback(err));
    } else {
      callback(null, room);
    }
  },

  








  create: function(room, callback) {
    if (!("roomName" in room) || !("expiresIn" in room) ||
        !("roomOwner" in room) || !("maxSize" in room)) {
      callback(new Error("Missing required property to create a room"));
      return;
    }

    let sessionType = MozLoopService.userProfile ? LOOP_SESSION_TYPE.FXA :
                      LOOP_SESSION_TYPE.GUEST;

    MozLoopService.hawkRequest(sessionType, "/rooms", "POST", room)
      .then(response => {
        let data = JSON.parse(response.body);
        extend(room, data);
        
        delete room.expiresIn;
        this.rooms.set(room.roomToken, room);

        eventEmitter.emit("add", room);
        callback(null, room);
      }, error => callback(error)).catch(error => callback(error));
  },

  





  onNotification: function(version, channelID) {
    gDirty = true;
    this.getAll(version, () => {});
  },
};
Object.freeze(LoopRoomsInternal);














this.LoopRooms = {
  getAll: function(version, callback) {
    return LoopRoomsInternal.getAll(version, callback);
  },

  get: function(roomToken, callback) {
    return LoopRoomsInternal.get(roomToken, callback);
  },

  create: function(options, callback) {
    return LoopRoomsInternal.create(options, callback);
  },

  promise: function(method, ...params) {
    return new Promise((resolve, reject) => {
      this[method](...params, (error, result) => {
        if (error) {
          reject(error);
        } else {
          resolve(result);
        }
      });
    });
  },

  on: (...params) => eventEmitter.on(...params),

  once: (...params) => eventEmitter.once(...params),

  off: (...params) => eventEmitter.off(...params)
};
Object.freeze(this.LoopRooms);
