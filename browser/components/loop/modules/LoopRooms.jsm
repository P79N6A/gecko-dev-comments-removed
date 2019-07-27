


"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Timer.jsm");

const {MozLoopService, LOOP_SESSION_TYPE} = Cu.import("resource:///modules/loop/MozLoopService.jsm", {});
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "CommonUtils",
                                  "resource://services-common/utils.js");
XPCOMUtils.defineLazyGetter(this, "eventEmitter", function() {
  const {EventEmitter} = Cu.import("resource://gre/modules/devtools/event-emitter.js", {});
  return new EventEmitter();
});
XPCOMUtils.defineLazyGetter(this, "gLoopBundle", function() {
  return Services.strings.createBundle('chrome://browser/locale/loop/loop.properties');
});

XPCOMUtils.defineLazyModuleGetter(this, "LoopRoomsCache",
  "resource:///modules/loop/LoopRoomsCache.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "loopUtils",
  "resource:///modules/loop/utils.js", "utils");
XPCOMUtils.defineLazyModuleGetter(this, "loopCrypto",
  "resource:///modules/loop/crypto.js", "LoopCrypto");


this.EXPORTED_SYMBOLS = ["LoopRooms", "roomsPushNotification"];


const CLIENT_MAX_SIZE = 2;


const MIN_TIME_BEFORE_ENCRYPTION = 5 * 1000;

const MAX_TIME_BEFORE_ENCRYPTION = 30 * 60 * 1000;

const TIME_BETWEEN_ENCRYPTIONS = 1000;

const roomsPushNotification = function(version, channelID) {
  return LoopRoomsInternal.onNotification(version, channelID);
};





let gDirty = true;

let gCurrentUser = null;

let gRoomsCache = null;







const extend = function(target, source) {
  for (let key of Object.getOwnPropertyNames(source)) {
    target[key] = source[key];
  }
  return target;
};











const containsParticipant = function(room, participant) {
  for (let user of room.participants) {
    if (user.roomConnectionId == participant.roomConnectionId) {
      return true;
    }
  }
  return false;
};












const checkForParticipantsUpdate = function(room, updatedRoom) {
  
  
  if (!("participants" in room)) {
    return;
  }

  let participant;
  
  for (participant of updatedRoom.participants) {
    if (!containsParticipant(room, participant)) {
      eventEmitter.emit("joined", room, participant);
      eventEmitter.emit("joined:" + room.roomToken, participant);
    }
  }

  
  for (participant of room.participants) {
    if (!containsParticipant(updatedRoom, participant)) {
      eventEmitter.emit("left", room, participant);
      eventEmitter.emit("left:" + room.roomToken, participant);
    }
  }
};





let timerHandlers = {
  






  startTimer(callback, delay) {
    return setTimeout(callback, delay);
  }
};








let LoopRoomsInternal = {
  


  rooms: new Map(),

  get roomsCache() {
    if (!gRoomsCache) {
      gRoomsCache = new LoopRoomsCache();
    }
    return gRoomsCache;
  },

  



  encryptionQueue: {
    queue: [],
    timer: null,
    reset: function() {
      this.queue = [];
      this.timer = null;
    }
  },

  


  get sessionType() {
    return MozLoopService.userProfile ? LOOP_SESSION_TYPE.FXA :
                                        LOOP_SESSION_TYPE.GUEST;
  },

  



  get participantsCount() {
    let count = 0;
    for (let room of this.rooms.values()) {
      if (room.deleted || !("participants" in room)) {
        continue;
      }
      count += room.participants.length;
    }
    return count;
  },

  






  processEncryptionQueue: Task.async(function* () {
    let roomToken = this.encryptionQueue.queue.shift();

    
    
    let roomData = this.rooms.get(roomToken);

    if (roomData) {
      try {
        
        
        yield LoopRooms.promise("update", roomToken, {});
      } catch (error) {
        MozLoopService.log.error("Upgrade encryption of room failed", error);
        
      }
    }

    if (this.encryptionQueue.queue.length) {
      this.encryptionQueue.timer =
        timerHandlers.startTimer(this.processEncryptionQueue.bind(this), TIME_BETWEEN_ENCRYPTIONS);
    } else {
      this.encryptionQueue.timer = null;
    }
  }),

  






  queueForEncryption: function(roomToken) {
    if (!this.encryptionQueue.queue.includes(roomToken)) {
      this.encryptionQueue.queue.push(roomToken);
    }

    
    
    
    
    
    if (!this.encryptionQueue.timer) {
      let waitTime = (MAX_TIME_BEFORE_ENCRYPTION - MIN_TIME_BEFORE_ENCRYPTION) *
        Math.random() + MIN_TIME_BEFORE_ENCRYPTION;
      this.encryptionQueue.timer =
        timerHandlers.startTimer(this.processEncryptionQueue.bind(this), waitTime);
    }
  },

  







  promiseGetOrCreateRoomKey: Task.async(function* (roomData) {
    if (roomData.roomKey) {
      return roomData.roomKey;
    }

    return yield loopCrypto.generateKey();
  }),

  






  promiseEncryptedRoomKey: Task.async(function* (key) {
    let profileKey = yield MozLoopService.promiseProfileEncryptionKey();

    let encryptedRoomKey = yield loopCrypto.encryptBytes(profileKey, key);
    return encryptedRoomKey;
  }),

  





  promiseDecryptRoomKey: Task.async(function* (encryptedKey) {
    let profileKey = yield MozLoopService.promiseProfileEncryptionKey();

    let decryptedRoomKey = yield loopCrypto.decryptBytes(profileKey, encryptedKey);
    return decryptedRoomKey;
  }),

  











  promiseEncryptRoomData: Task.async(function* (roomData) {
    
    
    
    function getUnencryptedData() {
      var serverRoomData = extend({}, roomData);
      delete serverRoomData.decryptedContext;

      
      serverRoomData.roomName = roomData.decryptedContext.roomName;

      return {
        all: roomData,
        encrypted: serverRoomData
      };
    }

    var newRoomData = extend({}, roomData);

    if (!newRoomData.context) {
      newRoomData.context = {};
    }

    
    let key = yield this.promiseGetOrCreateRoomKey(newRoomData);

    try {
      newRoomData.context.wrappedKey = yield this.promiseEncryptedRoomKey(key);
    }
    catch (ex) {
      
      
      if (ex.message == "FxA re-register not implemented") {
        return getUnencryptedData();
      }
      return Promise.reject(ex);
    }

    
    newRoomData.context.value = yield loopCrypto.encryptBytes(key,
      JSON.stringify(newRoomData.decryptedContext));

    
    
    newRoomData.context.alg = "AES-GCM";
    newRoomData.roomKey = key;

    var serverRoomData = extend({}, newRoomData);

    
    delete serverRoomData.decryptedContext;
    delete serverRoomData.roomKey;

    return {
      encrypted: serverRoomData,
      all: newRoomData
    };
  }),

  





  promiseDecryptRoomData: Task.async(function* (roomData) {
    if (!roomData.context) {
      return roomData;
    }

    if (!roomData.context.wrappedKey) {
      throw new Error("Missing wrappedKey");
    }

    let savedRoomKey = yield this.roomsCache.getKey(this.sessionType, roomData.roomToken);
    let fallback = false;
    let key;

    try {
      key = yield this.promiseDecryptRoomKey(roomData.context.wrappedKey);
    } catch (error) {
      
      if (!savedRoomKey) {
        throw error;
      }

      
      
      key = savedRoomKey;
      fallback = true;
    }

    let decryptedData = yield loopCrypto.decryptBytes(key, roomData.context.value);

    if (fallback) {
      
      
      MozLoopService.log.debug("Fell back to saved key, queuing for encryption",
        roomData.roomToken);
      this.queueForEncryption(roomData.roomToken);
    } else if (!savedRoomKey || key != savedRoomKey) {
      
      try {
        yield this.roomsCache.setKey(this.sessionType, roomData.roomToken, key);
      }
      catch (error) {
        MozLoopService.log.error("Failed to save room key:", error);
      }
    }

    roomData.roomKey = key;
    roomData.decryptedContext = JSON.parse(decryptedData);

    
    roomData.roomUrl = roomData.roomUrl.split("#")[0];
    
    roomData.roomUrl = roomData.roomUrl + "#" + roomData.roomKey;

    return roomData;
  }),

  





  saveAndNotifyUpdate: function(roomData, isUpdate) {
    this.rooms.set(roomData.roomToken, roomData);

    let eventName = isUpdate ? "update" : "add";
    eventEmitter.emit(eventName, roomData);
    eventEmitter.emit(eventName + ":" + roomData.roomToken, roomData);
  },

  









  addOrUpdateRoom: Task.async(function* (room, isUpdate) {
    if (!room.context) {
      
      

      
      
      
      room.decryptedContext = {
        roomName: room.roomName
      };
      delete room.roomName;

      
      
      this.queueForEncryption(room.roomToken);

      this.saveAndNotifyUpdate(room, isUpdate);
    } else {
      
      try {
        let roomData = yield this.promiseDecryptRoomData(room);

        this.saveAndNotifyUpdate(roomData, isUpdate);
      } catch (error) {
        MozLoopService.log.error("Failed to decrypt room data: ", error);
        
        room.decryptedContext = {};
        this.saveAndNotifyUpdate(room, isUpdate);
      }
    }
  }),

  









  getAll: function(version = null, callback = null) {
    if (!callback) {
      callback = version;
      version = null;
    }

    Task.spawn(function* () {
      if (!gDirty) {
        callback(null, [...this.rooms.values()]);
        return;
      }

      
      let url = "/rooms" + (version ? "?version=" + encodeURIComponent(version) : "");
      let response = yield MozLoopService.hawkRequest(this.sessionType, url, "GET");
      let roomsList = JSON.parse(response.body);
      if (!Array.isArray(roomsList)) {
        throw new Error("Missing array of rooms in response.");
      }

      for (let room of roomsList) {
        
        let orig = this.rooms.get(room.roomToken);

        if (room.deleted) {
          
          
          if (orig) {
            this.rooms.delete(room.roomToken);
          }

          eventEmitter.emit("delete", room);
          eventEmitter.emit("delete:" + room.roomToken, room);
        } else {
          if (orig) {
            checkForParticipantsUpdate(orig, room);
          }

          yield this.addOrUpdateRoom(room, !!orig);
        }
      }

      
      
      if (this.sessionType == LOOP_SESSION_TYPE.GUEST && !this.rooms.size) {
        this.setGuestCreatedRoom(false);
      }

      
      gDirty = false;
      callback(null, [...this.rooms.values()]);
    }.bind(this)).catch(error => {
      callback(error);
    });
  },

  









  get: function(roomToken, callback) {
    let room = this.rooms.has(roomToken) ? this.rooms.get(roomToken) : {};
    
    let needsUpdate = !("participants" in room);
    if (!gDirty && !needsUpdate) {
      
      
      callback(null, room);
      return;
    }

    Task.spawn(function* () {
      let response = yield MozLoopService.hawkRequest(this.sessionType,
        "/rooms/" + encodeURIComponent(roomToken), "GET");

      let data = JSON.parse(response.body);

      room.roomToken = roomToken;

      if (data.deleted) {
        this.rooms.delete(room.roomToken);

        extend(room, data);
        eventEmitter.emit("delete", room);
        eventEmitter.emit("delete:" + room.roomToken, room);
      } else {
        checkForParticipantsUpdate(room, data);
        extend(room, data);

        yield this.addOrUpdateRoom(room, !needsUpdate);
      }
      callback(null, room);
    }.bind(this)).catch(callback);
  },

  








  create: function(room, callback) {
    if (!("decryptedContext" in room) || !("roomOwner" in room) ||
        !("maxSize" in room)) {
      callback(new Error("Missing required property to create a room"));
      return;
    }

    Task.spawn(function* () {
      let {all, encrypted} = yield this.promiseEncryptRoomData(room);

      
      room = all;
      
      let response = yield MozLoopService.hawkRequest(this.sessionType, "/rooms",
        "POST", encrypted);

      extend(room, JSON.parse(response.body));
      
      delete room.expiresIn;
      this.rooms.set(room.roomToken, room);

      if (this.sessionType == LOOP_SESSION_TYPE.GUEST) {
        this.setGuestCreatedRoom(true);
      }

      
      yield this.roomsCache.setKey(this.sessionType, room.roomToken, room.roomKey);

      eventEmitter.emit("add", room);
      callback(null, room);
    }.bind(this)).catch(callback);
  },

  




  setGuestCreatedRoom: function(created) {
    if (created) {
      Services.prefs.setBoolPref("loop.createdRoom", created);
    } else {
      Services.prefs.clearUserPref("loop.createdRoom");
    }
  },

  


  getGuestCreatedRoom: function() {
    try {
      return Services.prefs.getBoolPref("loop.createdRoom");
    } catch (x) {
      return false;
    }
  },

  open: function(roomToken) {
    let windowData = {
      roomToken: roomToken,
      type: "room"
    };

    MozLoopService.openChatWindow(windowData);
  },

  







  delete: function(roomToken, callback) {
    
    
    let room = this.rooms.get(roomToken);
    let url = "/rooms/" + encodeURIComponent(roomToken);
    MozLoopService.hawkRequest(this.sessionType, url, "DELETE")
      .then(response => {
        this.rooms.delete(roomToken);
        eventEmitter.emit("delete", room);
        eventEmitter.emit("delete:" + room.roomToken, room);
        callback(null, room);
      }, error => callback(error)).catch(error => callback(error));
  },

  








  _postToRoom(roomToken, postData, callback) {
    let url = "/rooms/" + encodeURIComponent(roomToken);
    MozLoopService.hawkRequest(this.sessionType, url, "POST", postData).then(response => {
      
      var joinData = response.body ? JSON.parse(response.body) : {};
      callback(null, joinData);
    }, error => callback(error)).catch(error => callback(error));
  },

  







  join: function(roomToken, callback) {
    let displayName;
    if (MozLoopService.userProfile && MozLoopService.userProfile.email) {
      displayName = MozLoopService.userProfile.email;
    } else {
      displayName = gLoopBundle.GetStringFromName("display_name_guest");
    }

    this._postToRoom(roomToken, {
      action: "join",
      displayName: displayName,
      clientMaxSize: CLIENT_MAX_SIZE
    }, callback);
  },

  









  refreshMembership: function(roomToken, sessionToken, callback) {
    this._postToRoom(roomToken, {
      action: "refresh",
      sessionToken: sessionToken
    }, callback);
  },

  










  leave: function(roomToken, sessionToken, callback) {
    if (!callback) {
      callback = function(error) {
        if (error) {
          MozLoopService.log.error(error);
        }
      };
    }
    this._postToRoom(roomToken, {
      action: "leave",
      sessionToken: sessionToken
    }, callback);
  },

  











  sendConnectionStatus: function(roomToken, sessionToken, status, callback) {
    if (!callback) {
      callback = function(error) {
        if (error) {
          MozLoopService.log.error(error);
        }
      };
    }
    this._postToRoom(roomToken, {
      action: "status",
      event: status.event,
      state: status.state,
      connections: status.connections,
      sendStreams: status.sendStreams,
      recvStreams: status.recvStreams,
      sessionToken: sessionToken
    }, callback);
  },

  












  update: function(roomToken, roomData, callback) {
    let room = this.rooms.get(roomToken);
    let url = "/rooms/" + encodeURIComponent(roomToken);
    if (!room.decryptedContext) {
      room.decryptedContext = {
        roomName: roomData.roomName || room.roomName
      };
    } else {
      
      
      room.decryptedContext.roomName = roomData.roomName ||
                                       room.decryptedContext.roomName ||
                                       room.roomName;
    }
    if (roomData.urls && roomData.urls.length) {
      
      room.decryptedContext.urls = [roomData.urls[0]];
    }

    Task.spawn(function* () {
      let {all, encrypted} = yield this.promiseEncryptRoomData(room);

      
      let sendData = {
        context: encrypted.context
      };

      
      
      if (!sendData.context) {
        sendData = {
          roomName: room.decryptedContext.roomName
        };
      } else {
        
        
        yield this.roomsCache.setKey(this.sessionType, all.roomToken, all.roomKey);
      }

      let response = yield MozLoopService.hawkRequest(this.sessionType,
          url, "PATCH", sendData);

      let newRoomData = all;

      extend(newRoomData, JSON.parse(response.body));
      this.rooms.set(roomToken, newRoomData);
      callback(null, newRoomData);
    }.bind(this)).catch(callback);
  },

  





  onNotification: function(version, channelID) {
    
    let channelIDs = MozLoopService.channelIDs;
    if ((this.sessionType == LOOP_SESSION_TYPE.GUEST && channelID != channelIDs.roomsGuest) ||
        (this.sessionType == LOOP_SESSION_TYPE.FXA   && channelID != channelIDs.roomsFxA)) {
      return;
    }

    let oldDirty = gDirty;
    gDirty = true;
    
    
    
    this.getAll(oldDirty ? null : version, () => {});
  },

  





  maybeRefresh: function(user = null) {
    if (gCurrentUser == user) {
      return;
    }

    gCurrentUser = user;
    if (!gDirty) {
      gDirty = true;
      this.rooms.clear();
      eventEmitter.emit("refresh");
      this.getAll(null, () => {});
    }
  }
};
Object.freeze(LoopRoomsInternal);

















this.LoopRooms = {
  get participantsCount() {
    return LoopRoomsInternal.participantsCount;
  },

  getAll: function(version, callback) {
    return LoopRoomsInternal.getAll(version, callback);
  },

  get: function(roomToken, callback) {
    return LoopRoomsInternal.get(roomToken, callback);
  },

  create: function(options, callback) {
    return LoopRoomsInternal.create(options, callback);
  },

  open: function(roomToken) {
    return LoopRoomsInternal.open(roomToken);
  },

  delete: function(roomToken, callback) {
    return LoopRoomsInternal.delete(roomToken, callback);
  },

  join: function(roomToken, callback) {
    return LoopRoomsInternal.join(roomToken, callback);
  },

  refreshMembership: function(roomToken, sessionToken, callback) {
    return LoopRoomsInternal.refreshMembership(roomToken, sessionToken,
      callback);
  },

  leave: function(roomToken, sessionToken, callback) {
    return LoopRoomsInternal.leave(roomToken, sessionToken, callback);
  },

  sendConnectionStatus: function(roomToken, sessionToken, status, callback) {
    return LoopRoomsInternal.sendConnectionStatus(roomToken, sessionToken, status, callback);
  },

  update: function(roomToken, roomData, callback) {
    return LoopRoomsInternal.update(roomToken, roomData, callback);
  },

  getGuestCreatedRoom: function() {
    return LoopRoomsInternal.getGuestCreatedRoom();
  },

  maybeRefresh: function(user) {
    return LoopRoomsInternal.maybeRefresh(user);
  },

  





  stubCache: function(stub) {
    LoopRoomsInternal.rooms.clear();
    if (stub) {
      
      for (let [key, value] of stub.entries()) {
        LoopRoomsInternal.rooms.set(key, value);
      }
      gDirty = false;
    } else {
      
      
      gDirty = true;
    }
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
