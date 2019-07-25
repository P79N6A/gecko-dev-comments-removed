




































let EXPORTED_SYMBOLS = ["Preferences"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

function Preferences(prefBranch) {
  if (prefBranch)
    this._prefBranch = prefBranch;
}

Preferences.prototype = {
  _prefBranch: "",

  

  get _prefSvc() {
    let prefSvc = Cc["@mozilla.org/preferences-service;1"].
                  getService(Ci.nsIPrefService).
                  getBranch(this._prefBranch).
                  QueryInterface(Ci.nsIPrefBranch2);
    this.__defineGetter__("_prefSvc", function() prefSvc);
    return this._prefSvc;
  },

  







  get: function(prefName, defaultValue) {
    
    
    if (typeof prefName == "object" && prefName.constructor.name == Array.name)
      return prefName.map(function(v) this.get(v), this);

    switch (this._prefSvc.getPrefType(prefName)) {
      case Ci.nsIPrefBranch.PREF_STRING:
        return this._prefSvc.getCharPref(prefName);

      case Ci.nsIPrefBranch.PREF_INT:
        return this._prefSvc.getIntPref(prefName);

      case Ci.nsIPrefBranch.PREF_BOOL:
        return this._prefSvc.getBoolPref(prefName);

      case Ci.nsIPrefBranch.PREF_INVALID:
      default:
        return defaultValue;
    }
  },

  set: function(prefName, prefValue) {
    
    
    if (typeof prefName == "object" && prefName.constructor.name == Object.name)
      for (let [name, value] in Iterator(prefName))
        this.set(name, value);
    else {
      switch (typeof prefValue) {
        case "number":
          this._prefSvc.setIntPref(prefName, prefValue);
          break;

        case "boolean":
          this._prefSvc.setBoolPref(prefName, prefValue);
          break;

        case "string":
        default:
          this._prefSvc.setCharPref(prefName, prefValue);
          break;
      }
    }
  },

  

  has: function(prefName) {
    return (this._prefSvc.getPrefType(prefName) != Ci.nsIPrefBranch.PREF_INVALID);
  },

  modified: function(prefName) {
    return (this.has(prefName) && this._prefSvc.prefHasUserValue(prefName));
  },

  locked: function(prefName) {
    return this._prefSvc.isLocked(prefName);
  },

  lock: function(prefName) {
    this._prefSvc.lockPref(prefName);
  },

  unlock: function(prefName) {
    this._prefSvc.unlockPref(prefName);
  },

  reset: function(prefName) {
    this._prefSvc.clearUserPref(prefName);
  },

  resetBranch: function(prefBranch) {
    this._prefSvc.resetBranch(prefBranch);
  }

};




Preferences.__proto__ = Preferences.prototype;
