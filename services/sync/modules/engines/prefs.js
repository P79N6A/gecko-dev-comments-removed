



































const EXPORTED_SYMBOLS = ['PrefsEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const WEAVE_SYNC_PREFS = "extensions.weave.prefs.sync";
const WEAVE_PREFS_GUID = "preferences";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

Cu.import("resource://weave/log4moz.js");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/type_records/prefs.js");

function PrefsEngine() {
  this._init();
}
PrefsEngine.prototype = {
  __proto__: SyncEngine.prototype,
  name: "prefs",
  displayName: "Preferences",
  description: "Synchronize your home page, selected persona, and more",
  logName: "Prefs",
  _storeObj: PrefStore,
  _trackerObj: PrefTracker,
  _recordObj: PrefRec,
};


function PrefStore() {
  this._init();
}
PrefStore.prototype = {
  __proto__: Store.prototype,
  name: "prefs",
  _logName: "PrefStore",

  get _prefs() {
    let prefs = Cc["@mozilla.org/preferences-service;1"].
      getService(Ci.nsIPrefBranch);

    this.__defineGetter__("_prefs", function() prefs);
    return prefs;
  },

  get _syncPrefs() {
    let service = Cc["@mozilla.org/preferences-service;1"].
      getService(Ci.nsIPrefService);
    let syncPrefs = service.getBranch(WEAVE_SYNC_PREFS).getChildList("", {}).
      map(function(elem) { return elem.substr(1); });
    
    this.__defineGetter__("_syncPrefs", function() syncPrefs);
    return syncPrefs;
  },
  
  _getAllPrefs: function PrefStore__getAllPrefs() {
    let values = [];
    let toSync = this._syncPrefs;
    
    let pref;
    for (let i = 0; i < toSync.length; i++) {
      if (!this._prefs.getBoolPref(WEAVE_SYNC_PREFS + "." + toSync[i]))
        continue;
        
      pref = {};
      pref["name"] = toSync[i];

      switch (this._prefs.getPrefType(toSync[i])) {
        case Ci.nsIPrefBranch.PREF_INT:
          pref["type"] = "int";
          pref["value"] = this._prefs.getIntPref(toSync[i]);
          break;
        case Ci.nsIPrefBranch.PREF_STRING:
          pref["type"] = "string";
          pref["value"] = this._prefs.getCharPref(toSync[i]);
          break;
        case Ci.nsIPrefBranch.PREF_BOOL:
          pref["type"] = "boolean";
          pref["value"] = this._prefs.getBoolPref(toSync[i]);
          break;
        default:
          this._log.trace("Unsupported pref type for " + toSync[i]);
      }
      if ("value" in pref)
        values[values.length] = pref;
    }
    
    return values;
  },
  
  _setAllPrefs: function PrefStore__setAllPrefs(values) {
    for (let i = 0; i < values.length; i++) {
      switch (values[i]["type"]) {
        case "int":
          this._prefs.setIntPref(values[i]["name"], values[i]["value"]);
          break;
        case "string":
          this._prefs.setCharPref(values[i]["name"], values[i]["value"]);
          break;
        case "boolean":
          this._prefs.setBoolPref(values[i]["name"], values[i]["value"]);
          break;
        default:
          this._log.trace("Unexpected preference type: " + values[i]["type"]);
      }
    }
  },
  
  getAllIDs: function PrefStore_getAllIDs() {
    
    let allprefs = {};
    allprefs[WEAVE_PREFS_GUID] = this._getAllPrefs();
    return allprefs;
  },
  
  changeItemID: function PrefStore_changeItemID(oldID, newID) {
    this._log.trace("PrefStore GUID is constant!");
  },
  
  itemExists: function FormStore_itemExists(id) {
    return (id === WEAVE_PREFS_GUID);
  },
  
  createRecord: function FormStore_createRecord(guid, cryptoMetaURL) {
    let record = new PrefRec();
    record.id = guid;
    
    if (guid == WEAVE_PREFS_GUID) {
      record.encryption = cryptoMetaURL;
      record.value = this._getAllPrefs();
    } else {
      record.deleted = true;
    }
    
    return record;
  },
  
  create: function PrefStore_create(record) {
    this._log.trace("Ignoring create request");
  },

  remove: function PrefStore_remove(record) {
    this._log.trace("Ignoring remove request")
  },

  update: function PrefStore_update(record) {
    this._log.trace("Received pref updates, applying...");
    this._setAllPrefs(record.value);
  },
  
  wipe: function PrefStore_wipe() {
    this._log.trace("Ignoring wipe request");
  }
};

function PrefTracker() {
  this._init();
}
PrefTracker.prototype = {
  __proto__: Tracker.prototype,
  name: "prefs",
  _logName: "PrefTracker",
  file: "prefs",
  
  get _prefs() {
    let prefs = Cc["@mozilla.org/preferences-service;1"].
      getService(Ci.nsIPrefBranch2);

    this.__defineGetter__("_prefs", function() prefs);
    return prefs;
  },

  get _syncPrefs() {
    let service = Cc["@mozilla.org/preferences-service;1"].
      getService(Ci.nsIPrefService);
    let syncPrefs = service.getBranch(WEAVE_SYNC_PREFS).getChildList("", {}).
      map(function(elem) { return elem.substr(1); });
    
    this.__defineGetter__("_syncPrefs", function() syncPrefs);
    return syncPrefs;
  },
  
  _init: function PrefTracker__init() {
    this.__proto__.__proto__._init.call(this);
    this._prefs.addObserver("", this, false);   
  },
  
  
  observe: function(aSubject, aTopic, aData) {
    if (aTopic != "nsPref:changed")
      return;
    
    if (this._syncPrefs.indexOf(aData) != -1) {
      this._score += 25;
      this.addChangedID(WEAVE_PREFS_GUID);
      this._log.trace("Preference " + aData + " changed");
    }
  }
};
