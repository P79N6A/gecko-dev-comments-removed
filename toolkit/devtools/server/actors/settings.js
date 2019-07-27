



const {Cc, Ci, Cu, CC} = require("chrome");
const protocol = require("devtools/server/protocol");
const {Arg, method, RetVal} = protocol;
const {DebuggerServer} = require("devtools/server/main");
const {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});

Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/Services.jsm");

let defaultSettings = {};
let settingsFile;

exports.register = function(handle) {
  handle.addGlobalActor(SettingsActor, "settingsActor");
};

exports.unregister = function(handle) {
};

function getDefaultSettings() {
  let chan = NetUtil.newChannel2(settingsFile,
                                 null,
                                 null,
                                 null,      
                                 Services.scriptSecurityManager.getSystemPrincipal(),
                                 null,      
                                 Ci.nsILoadInfo.SEC_NORMAL,
                                 Ci.nsIContentPolicy.TYPE_OTHER);
  let stream = chan.open();
  
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                  .createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  let rawstr = converter.ConvertToUnicode(NetUtil.readInputStreamToString(
                                          stream,
                                          stream.available()) || "");

  try {
    defaultSettings = JSON.parse(rawstr);
  } catch(e) { }
  stream.close();
}

function loadSettingsFile() {
  
  
  
  if (settingsFile) {
    return;
  }
  settingsFile = FileUtils.getFile("DefRt", ["settings.json"], false);
  if (!settingsFile || (settingsFile && !settingsFile.exists())) {
    
    
    settingsFile = FileUtils.getFile("ProfD", ["settings.json"], false);
    if (!settingsFile || (settingsFile && !settingsFile.exists())) {
      console.log("settings.json file does not exist");
    }
  }

  if (settingsFile.exists()) {
    getDefaultSettings();
  }
}

let SettingsActor = exports.SettingsActor = protocol.ActorClass({
  typeName: "settings",

  _getSettingsService: function() {
    let win = Services.wm.getMostRecentWindow(DebuggerServer.chromeWindowType);
    return win.navigator.mozSettings;
  },

  getSetting: method(function(name) {
    let deferred = promise.defer();
    let lock = this._getSettingsService().createLock();
    let req = lock.get(name);
    req.onsuccess = function() {
      deferred.resolve(req.result[name]);
    };
    req.onerror = function() {
      deferred.reject(req.error);
    };
    return deferred.promise;
  }, {
    request: { value: Arg(0) },
    response: { value: RetVal("json") }
  }),

  setSetting: method(function(name, value) {
    let deferred = promise.defer();
    let data = {};
    data[name] = value;
    let lock = this._getSettingsService().createLock();
    let req = lock.set(data);
    req.onsuccess = function() {
      deferred.resolve(true);
    };
    req.onerror = function() {
      deferred.reject(req.error);
    };
    return deferred.promise;
  }, {
    request: { name: Arg(0), value: Arg(1) },
    response: {}
  }),

  _hasUserSetting: function(name, value) {
    if (typeof value === "object") {
      return JSON.stringify(defaultSettings[name]) !== JSON.stringify(value);
    }
    return (defaultSettings[name] !== value);
  },

  getAllSettings: method(function() {
    loadSettingsFile();
    let settings = {};
    let self = this;

    let deferred = promise.defer();
    let lock = this._getSettingsService().createLock();
    let req = lock.get("*");

    req.onsuccess = function() {
      for (var name in req.result) {
        settings[name] = {
          value: req.result[name],
          hasUserValue: self._hasUserSetting(name, req.result[name])
        };
      }
      deferred.resolve(settings);
    };
    req.onfailure = function() {
      deferred.reject(req.error);
    };

    return deferred.promise;
  }, {
    request: {},
    response: { value: RetVal("json") }
  }),

  clearUserSetting: method(function(name) {
    loadSettingsFile();
    try {
      this.setSetting(name, defaultSettings[name]);
    } catch (e) {
      console.log(e);
    }
  }, {
    request: { name: Arg(0) },
    response: {}
  })
});

let SettingsFront = protocol.FrontClass(SettingsActor, {
  initialize: function(client, form) {
    protocol.Front.prototype.initialize.call(this, client);
    this.actorID = form.settingsActor;
    this.manage(this);
  },
});

const _knownSettingsFronts = new WeakMap();

exports.getSettingsFront = function(client, form) {
  if (!form.settingsActor) {
    return null;
  }
  if (_knownSettingsFronts.has(client)) {
    return _knownSettingsFronts.get(client);
  }
  let front = new SettingsFront(client, form);
  _knownSettingsFronts.set(client, front);
  return front;
};


exports._setDefaultSettings = function(settings) {
  defaultSettings = settings || {};
};
