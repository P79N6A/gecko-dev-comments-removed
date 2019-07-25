





































var jsm = {}; Cu.import("resource://gre/modules/XPCOMUtils.jsm", jsm);
var XPCOMUtils = jsm.XPCOMUtils;





const MAX_INT = Math.pow(2, 31) - 1;
const MIN_INT = -MAX_INT;

var prefSvc = Cc["@mozilla.org/preferences-service;1"].
              getService(Ci.nsIPrefService).getBranch(null);

var get = exports.get = function get(name, defaultValue) {
  switch (prefSvc.getPrefType(name)) {
  case Ci.nsIPrefBranch.PREF_STRING:
    return prefSvc.getComplexValue(name, Ci.nsISupportsString).data;

  case Ci.nsIPrefBranch.PREF_INT:
    return prefSvc.getIntPref(name);

  case Ci.nsIPrefBranch.PREF_BOOL:
    return prefSvc.getBoolPref(name);

  case Ci.nsIPrefBranch.PREF_INVALID:
    return defaultValue;

  default:
    
    throw new Error("Error getting pref " + name +
                    "; its value's type is " +
                    prefSvc.getPrefType(name) +
                    ", which I don't know " +
                    "how to handle.");
  }
};

var set = exports.set = function set(name, value) {
  var prefType;
  if (typeof value != "undefined" && value != null)
    prefType = value.constructor.name;

  switch (prefType) {
  case "String":
    {
      var string = Cc["@mozilla.org/supports-string;1"].
                   createInstance(Ci.nsISupportsString);
      string.data = value;
      prefSvc.setComplexValue(name, Ci.nsISupportsString, string);
    }
    break;

  case "Number":
    
    
    
    
    if (value > MAX_INT || value < MIN_INT)
      throw new Error("you cannot set the " + name +
                      " pref to the number " + value +
                      ", as number pref values must be in the signed " +
                      "32-bit integer range -(2^31-1) to 2^31-1.  " +
                      "To store numbers outside that range, store " +
                      "them as strings.");
    if (value % 1 != 0)
      throw new Error("cannot store non-integer number: " + value);
    prefSvc.setIntPref(name, value);
    break;

  case "Boolean":
    prefSvc.setBoolPref(name, value);
    break;

  default:
    throw new Error("can't set pref " + name + " to value '" + value +
                    "'; it isn't a String, Number, or Boolean");
  }
};

var has = exports.has = function has(name) {
  return (prefSvc.getPrefType(name) != Ci.nsIPrefBranch.PREF_INVALID);
};

var isSet = exports.isSet = function isSet(name) {
  return (has(name) && prefSvc.prefHasUserValue(name));
};

var reset = exports.reset = function reset(name) {
  try {
    prefSvc.clearUserPref(name);
  } catch (e if e.result == Cr.NS_ERROR_UNEXPECTED) {
    
    
    
    
    
    
    
  }
};
