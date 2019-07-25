



































const EXPORTED_SYMBOLS = ['PrefsEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const WEAVE_SYNC_PREFS = "extensions.weave.prefs.sync.";
const WEAVE_PREFS_GUID = "preferences";

Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/type_records/prefs.js");

function PrefsEngine() {
  SyncEngine.call(this, "Prefs");
}
PrefsEngine.prototype = {
  __proto__: SyncEngine.prototype,
  _storeObj: PrefStore,
  _trackerObj: PrefTracker,
  _recordObj: PrefRec,

  _wipeClient: function _wipeClient() {
    SyncEngine.prototype._wipeClient.call(this);
    this.justWiped = true;
  },

  _reconcile: function _reconcile(item) {
    
    if (this.justWiped) {
      this.justWiped = false;
      return true;
    }
    return SyncEngine.prototype._reconcile.call(this, item);
  }
};


function PrefStore(name) {
  Store.call(this, name);
}
PrefStore.prototype = {
  __proto__: Store.prototype,

  get _prefs() {
    let prefs = Cc["@mozilla.org/preferences-service;1"].
      getService(Ci.nsIPrefBranch);

    this.__defineGetter__("_prefs", function() prefs);
    return prefs;
  },

  get _syncPrefs() {
    let service = Cc["@mozilla.org/preferences-service;1"].
      getService(Ci.nsIPrefService);
    let syncPrefs = service.getBranch(WEAVE_SYNC_PREFS).getChildList("", {});

    this.__defineGetter__("_syncPrefs", function() syncPrefs);
    return syncPrefs;
  },

  _getAllPrefs: function PrefStore__getAllPrefs() {
    let values = [];
    let toSync = this._syncPrefs;

    let pref;
    for (let i = 0; i < toSync.length; i++) {
      if (!this._prefs.getBoolPref(WEAVE_SYNC_PREFS + toSync[i]))
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

    
    try {
      let ltm = {};
      Cu.import("resource://gre/modules/LightweightThemeManager.jsm", ltm);
      ltm = ltm.LightweightThemeManager;
      if (ltm.currentTheme) {
        ltm.currentTheme = null;
        ltm.currentTheme = ltm.usedThemes[0];
      }
    }
    
    catch (ex) {}
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

  createRecord: function createRecord(guid) {
    let record = new PrefRec();

    if (guid == WEAVE_PREFS_GUID) {
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

function PrefTracker(name) {
  Tracker.call(this, name);
  this._prefs.addObserver("", this, false);
}
PrefTracker.prototype = {
  __proto__: Tracker.prototype,

  get _prefs() {
    let prefs = Cc["@mozilla.org/preferences-service;1"].
      getService(Ci.nsIPrefBranch2);

    this.__defineGetter__("_prefs", function() prefs);
    return prefs;
  },

  get _syncPrefs() {
    let service = Cc["@mozilla.org/preferences-service;1"].
      getService(Ci.nsIPrefService);
    let syncPrefs = service.getBranch(WEAVE_SYNC_PREFS).getChildList("", {});

    this.__defineGetter__("_syncPrefs", function() syncPrefs);
    return syncPrefs;
  },

  
  observe: function(aSubject, aTopic, aData) {
    if (aTopic != "nsPref:changed")
      return;

    if (this._syncPrefs.indexOf(aData) != -1) {
      this.score += 1;
      this.addChangedID(WEAVE_PREFS_GUID);
      this._log.trace("Preference " + aData + " changed");
    }
  }
};
