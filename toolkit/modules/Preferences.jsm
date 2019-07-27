



this.EXPORTED_SYMBOLS = ["Preferences"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");





const MAX_INT = 0x7FFFFFFF; 
const MIN_INT = -0x80000000;

this.Preferences =
  function Preferences(args) {
    this._cachedPrefBranch = null;
    if (isObject(args)) {
      if (args.branch)
        this._branchStr = args.branch;
      if (args.defaultBranch)
        this._defaultBranch = args.defaultBranch;
      if (args.privacyContext)
        this._privacyContext = args.privacyContext;
    }
    else if (args)
      this._branchStr = args;
  };

  













  Preferences.get = function(prefName, defaultValue, valueType = Ci.nsISupportsString) {
    if (Array.isArray(prefName))
      return prefName.map(function(v) this.get(v, defaultValue), this);

    return this._get(prefName, defaultValue, valueType);
  };

  Preferences._get = function(prefName, defaultValue, valueType) {
    switch (this._prefBranch.getPrefType(prefName)) {
      case Ci.nsIPrefBranch.PREF_STRING:
        return this._prefBranch.getComplexValue(prefName, valueType).data;

      case Ci.nsIPrefBranch.PREF_INT:
        return this._prefBranch.getIntPref(prefName);

      case Ci.nsIPrefBranch.PREF_BOOL:
        return this._prefBranch.getBoolPref(prefName);

      case Ci.nsIPrefBranch.PREF_INVALID:
        return defaultValue;

      default:
        
        throw "Error getting pref " + prefName + "; its value's type is " +
              this._prefBranch.getPrefType(prefName) + ", which I don't " +
              "know how to handle.";
    }
  };

  





















  Preferences.set = function(prefName, prefValue) {
    if (isObject(prefName)) {
      for (let [name, value] in Iterator(prefName))
        this.set(name, value);
      return;
    }

    this._set(prefName, prefValue);
  };

  Preferences._set = function(prefName, prefValue) {
    let prefType;
    if (typeof prefValue != "undefined" && prefValue != null)
      prefType = prefValue.constructor.name;

    switch (prefType) {
      case "String":
        {
          let str = Cc["@mozilla.org/supports-string;1"].
                       createInstance(Ci.nsISupportsString);
          str.data = prefValue;
          this._prefBranch.setComplexValue(prefName, Ci.nsISupportsString, str);
        }
        break;

      case "Number":
        
        
        
        
        if (prefValue > MAX_INT || prefValue < MIN_INT)
          throw("you cannot set the " + prefName + " pref to the number " +
                prefValue + ", as number pref values must be in the signed " +
                "32-bit integer range -(2^31-1) to 2^31-1.  To store numbers " +
                "outside that range, store them as strings.");
        this._prefBranch.setIntPref(prefName, prefValue);
        if (prefValue % 1 != 0)
          Cu.reportError("Warning: setting the " + prefName + " pref to the " +
                         "non-integer number " + prefValue + " converted it " +
                         "to the integer number " + this.get(prefName) +
                         "; to retain fractional precision, store non-integer " +
                         "numbers as strings.");
        break;

      case "Boolean":
        this._prefBranch.setBoolPref(prefName, prefValue);
        break;

      default:
        throw "can't set pref " + prefName + " to value '" + prefValue +
              "'; it isn't a String, Number, or Boolean";
    }
  };

  













  Preferences.has = function(prefName) {
    if (Array.isArray(prefName))
      return prefName.map(this.has, this);

    return (this._prefBranch.getPrefType(prefName) != Ci.nsIPrefBranch.PREF_INVALID);
  };

  













  Preferences.isSet = function(prefName) {
    if (Array.isArray(prefName))
      return prefName.map(this.isSet, this);

    return (this.has(prefName) && this._prefBranch.prefHasUserValue(prefName));
  },

  




  Preferences.modified = function(prefName) { return this.isSet(prefName) },

  Preferences.reset = function(prefName) {
    if (Array.isArray(prefName)) {
      prefName.map(function(v) this.reset(v), this);
      return;
    }

    this._prefBranch.clearUserPref(prefName);
  };

  





  Preferences.lock = function(prefName) {
    if (Array.isArray(prefName))
      prefName.map(this.lock, this);

    this._prefBranch.lockPref(prefName);
  };

  





  Preferences.unlock = function(prefName) {
    if (Array.isArray(prefName))
      prefName.map(this.unlock, this);

    this._prefBranch.unlockPref(prefName);
  };

  










  Preferences.locked = function(prefName) {
    if (Array.isArray(prefName))
      return prefName.map(this.locked, this);

    return this._prefBranch.prefIsLocked(prefName);
  };

  

















  Preferences.observe = function(prefName, callback, thisObject) {
    let fullPrefName = this._branchStr + (prefName || "");

    let observer = new PrefObserver(fullPrefName, callback, thisObject);
    Preferences._prefBranch.addObserver(fullPrefName, observer, true);
    observers.push(observer);

    return observer;
  };

  





















  Preferences.ignore = function(prefName, callback, thisObject) {
    let fullPrefName = this._branchStr + (prefName || "");

    
    
    
    
    let [observer] = observers.filter(function(v) v.prefName   == fullPrefName &&
                                                  v.callback   == callback &&
                                                  v.thisObject == thisObject);

    if (observer) {
      Preferences._prefBranch.removeObserver(fullPrefName, observer);
      observers.splice(observers.indexOf(observer), 1);
    }
  };

  Preferences.resetBranch = function(prefBranch = "") {
    try {
      this._prefBranch.resetBranch(prefBranch);
    }
    catch(ex) {
      
      
      if (ex.result == Cr.NS_ERROR_NOT_IMPLEMENTED)
        this.reset(this._prefBranch.getChildList(prefBranch, []));
      else
        throw ex;
    }
  },

  




  Preferences._branchStr = "";

  




  Preferences._cachedPrefBranch = null;

  



  Object.defineProperty(Preferences, "_prefBranch",
  {
    get: function _prefBranch() {
      if (!this._cachedPrefBranch) {
        let prefSvc = Services.prefs;
        this._cachedPrefBranch = this._defaultBranch ?
                                 prefSvc.getDefaultBranch(this._branchStr) :
                                 prefSvc.getBranch(this._branchStr);
      }
      return this._cachedPrefBranch;
    },
    enumerable: true,
    configurable: true
  });





Preferences.prototype = Preferences;












let observers = [];

function PrefObserver(prefName, callback, thisObject) {
  this.prefName = prefName;
  this.callback = callback;
  this.thisObject = thisObject;
}

PrefObserver.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver, Ci.nsISupportsWeakReference]),

  observe: function(subject, topic, data) {
    
    
    
    if (data.indexOf(this.prefName) != 0)
      return;

    if (typeof this.callback == "function") {
      let prefValue = Preferences.get(data);

      if (this.thisObject)
        this.callback.call(this.thisObject, prefValue);
      else
        this.callback(prefValue);
    }
    else 
      this.callback.observe(subject, topic, data);
  }
};

function isObject(val) {
  
  
  
  return (typeof val != "undefined" && val != null && typeof val == "object" &&
          val.constructor.name == "Object");
}
