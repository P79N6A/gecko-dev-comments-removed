


"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");
const {MozLoopService, LOOP_SESSION_TYPE} =
  Cu.import("resource:///modules/loop/MozLoopService.jsm", {});
XPCOMUtils.defineLazyModuleGetter(this, "CommonUtils",
                                  "resource://services-common/utils.js");
XPCOMUtils.defineLazyModuleGetter(this, "OS", "resource://gre/modules/osfile.jsm");

this.EXPORTED_SYMBOLS = ["LoopRoomsCache"];

const LOOP_ROOMS_CACHE_FILENAME = "loopRoomsCache.json";
























function LoopRoomsCache(options) {
  options = options || {};

  this.baseDir = options.baseDir || OS.Constants.Path.profileDir;
  this.path = OS.Path.join(
    this.baseDir,
    options.filename || LOOP_ROOMS_CACHE_FILENAME
  );
  this._cache = null;
}

LoopRoomsCache.prototype = {
  





  _setCache: function(contents) {
    this._cache = contents;

    return OS.File.makeDir(this.baseDir, {ignoreExisting: true}).then(() => {
        return CommonUtils.writeJSON(contents, this.path);
      });
  },

  





  _getCache: Task.async(function* () {
    if (this._cache) {
      return this._cache;
    }

    try {
      return (this._cache = yield CommonUtils.readJSON(this.path));
    } catch(error) {
      
      if ((OS.Constants.libc && error.unixErrno != OS.Constants.libc.ENOENT) ||
          (OS.Constants.Win && error.winLastError != OS.Constants.Win.ERROR_FILE_NOT_FOUND)) {
        MozLoopService.log.debug("Error reading the cache:", error);
      }
      return (this._cache = {});
    }
  }),

  




  clear: function() {
    this._cache = null;
    return OS.File.remove(this.path)
  },

  







  getKey: Task.async(function* (sessionType, roomToken) {
    if (sessionType != LOOP_SESSION_TYPE.FXA) {
      return null;
    }

    let sessionData = (yield this._getCache())[sessionType];

    if (!sessionData || !sessionData[roomToken]) {
      return null;
    }
    return sessionData[roomToken].key;
  }),

  








  setKey: Task.async(function* (sessionType, roomToken, roomKey) {
    if (sessionType != LOOP_SESSION_TYPE.FXA) {
      return;
    }

    let cache = yield this._getCache();

    
    
    
    
    if (!cache[sessionType]) {
      cache[sessionType] = {};
    }

    if (!cache[sessionType][roomToken]) {
      cache[sessionType][roomToken] = {};
    }

    
    if (!cache[sessionType][roomToken].key ||
        cache[sessionType][roomToken].key != roomKey) {
      cache[sessionType][roomToken].key = roomKey;
      return yield this._setCache(cache);
    }
  })
};
