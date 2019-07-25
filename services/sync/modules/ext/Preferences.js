




































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
    if (isArray(prefName))
      return prefName.map(function(v) this.get(v), this);

    switch (this._prefSvc.getPrefType(prefName)) {
      case Ci.nsIPrefBranch.PREF_STRING:
        return this._prefSvc.getComplexValue(prefName, Ci.nsISupportsString).data;

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
    if (isObject(prefName)) {
      for (let [name, value] in Iterator(prefName))
        this.set(name, value);
      return;
    }

    switch (typeof prefValue) {
      case "number":
        this._prefSvc.setIntPref(prefName, prefValue);
        if (prefValue % 1 != 0)
          Cu.reportError("WARNING: setting " + prefName + " pref to non-integer number " +
                         prefValue + " converts it to integer number " + this.get(prefName) +
                         "; to retain precision, store non-integer numbers as strings");
        break;

      case "boolean":
        this._prefSvc.setBoolPref(prefName, prefValue);
        break;

      case "string":
      default: {
        let string = Cc["@mozilla.org/supports-string;1"].
                     createInstance(Ci.nsISupportsString);
        string.data = prefValue;
        this._prefSvc.setComplexValue(prefName, Ci.nsISupportsString, string);
        break;
      }
    }
  },

  reset: function(prefName) {
    if (isArray(prefName)) {
      prefName.map(function(v) this.reset(v), this);
      return;
    }

    try {
      this._prefSvc.clearUserPref(prefName);
    }
    catch(ex) {
      
      
      
      
      
      
      
      if (ex.result != Cr.NS_ERROR_UNEXPECTED)
        throw ex;
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

  resetBranch: function(prefBranch) {
    try {
      this._prefSvc.resetBranch(prefBranch);
    }
    catch(ex) {
      
      
      if (ex.result == Cr.NS_ERROR_NOT_IMPLEMENTED)
        this.reset(this._prefSvc.getChildList(prefBranch, []));
      else
        throw ex;
    }
  }

};




Preferences.__proto__ = Preferences.prototype;

function isArray(val) {
  
  
  return (typeof val == "object" && val.constructor.name == Array.name);
}

function isObject(val) {
  
  
  return (typeof val == "object" && val.constructor.name == Object.name);
}
