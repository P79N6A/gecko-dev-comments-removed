# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:













































this.G_Preferences =
function G_Preferences(opt_startPoint, opt_getDefaultBranch) {
  this.debugZone = "prefs";
  this.observers_ = {};
  this.getDefaultBranch_ = !!opt_getDefaultBranch;

  this.startPoint_ = opt_startPoint || null;
}

G_Preferences.setterMap_ = { "string": "setCharPref",
                             "boolean": "setBoolPref",
                             "number": "setIntPref" };

G_Preferences.getterMap_ = {};
G_Preferences.getterMap_[Ci.nsIPrefBranch.PREF_STRING] = "getCharPref";
G_Preferences.getterMap_[Ci.nsIPrefBranch.PREF_BOOL] = "getBoolPref";
G_Preferences.getterMap_[Ci.nsIPrefBranch.PREF_INT] = "getIntPref";

G_Preferences.prototype.__defineGetter__('prefs_', function() {
  var prefs;
  var prefSvc = Cc["@mozilla.org/preferences-service;1"]
                  .getService(Ci.nsIPrefService);

  if (this.getDefaultBranch_) {
    prefs = prefSvc.getDefaultBranch(this.startPoint_);
  } else {
    prefs = prefSvc.getBranch(this.startPoint_);
  }

  
  prefs.QueryInterface(Ci.nsIPrefBranchInternal);
  return prefs;
});






G_Preferences.prototype.setPref = function(key, val) {
  var datatype = typeof(val);

  if (datatype == "number" && (val % 1 != 0)) {
    throw new Error("Cannot store non-integer numbers in preferences.");
  }

  var meth = G_Preferences.setterMap_[datatype];

  if (!meth) {
    throw new Error("Pref datatype {" + datatype + "} not supported.");
  }

  return this.prefs_[meth](key, val);
}






G_Preferences.prototype.getPref = function(key, opt_default) {
  var type = this.prefs_.getPrefType(key);

  
  if (type == Ci.nsIPrefBranch.PREF_INVALID) {
    return opt_default;
  }

  var meth = G_Preferences.getterMap_[type];

  if (!meth) {
    throw new Error("Pref datatype {" + type + "} not supported.");
  }

  
  
  try {
    return this.prefs_[meth](key);
  } catch(e) {
    return opt_default;
  }
}






G_Preferences.prototype.clearPref = function(which) {
  try {
    
    
    this.prefs_.clearUserPref(which);
  } catch(e) {}
}









G_Preferences.prototype.addObserver = function(which, callback) {
  
  if (!this.observers_[which])
    this.observers_[which] = { callbacks: [], observers: [] };

  
  if (this.observers_[which].callbacks.indexOf(callback) == -1) {
    var observer = new G_PreferenceObserver(callback);
    this.observers_[which].callbacks.push(callback);
    this.observers_[which].observers.push(observer);
    this.prefs_.addObserver(which, observer, false );
  }
}







G_Preferences.prototype.removeObserver = function(which, callback) {
  var ix = this.observers_[which].callbacks.indexOf(callback);
  G_Assert(this, ix != -1, "Tried to unregister a nonexistent observer"); 
  this.observers_[which].callbacks.splice(ix, 1);
  var observer = this.observers_[which].observers.splice(ix, 1)[0];
  this.prefs_.removeObserver(which, observer);
}




G_Preferences.prototype.removeAllObservers = function() {
  for (var which in this.observers_) {
    for each (var observer in this.observers_[which].observers) {
      this.prefs_.removeObserver(which, observer);
    }
  }
  this.observers_ = {};
}








this.G_PreferenceObserver =
function G_PreferenceObserver(callback) {
  this.debugZone = "prefobserver";
  this.callback_ = callback;
}











G_PreferenceObserver.prototype.observe = function(subject, topic, data) {
  G_Debug(this, "Observed pref change: " + data);
  this.callback_(data);
}






G_PreferenceObserver.prototype.QueryInterface = function(iid) {
  if (iid.equals(Ci.nsISupports) || 
      iid.equals(Ci.nsIObserver) ||
      iid.equals(Ci.nsISupportsWeakReference))
    return this;
  throw Components.results.NS_ERROR_NO_INTERFACE;
}

#ifdef DEBUG

this.TEST_G_Preferences = function TEST_G_Preferences() {
  if (G_GDEBUG) {
    var z = "preferences UNITTEST";
    G_debugService.enableZone(z);
    G_Debug(z, "Starting");

    var p = new G_Preferences();
    
    var testPref = "test-preferences-unittest";
    var noSuchPref = "test-preferences-unittest-aypabtu";
    
    
    var observeCount = 0;
    let observe = function (prefChanged) {
      G_Assert(z, prefChanged == testPref, "observer broken");
      observeCount++;
    };

    
    p.addObserver(testPref, observe);
    p.setPref(testPref, true);
    G_Assert(z, p.getPref(testPref), "get or set broken");
    G_Assert(z, observeCount == 1, "observer adding not working");

    p.removeObserver(testPref, observe);

    p.setPref(testPref, false);
    G_Assert(z, observeCount == 1, "observer removal not working");
    G_Assert(z, !p.getPref(testPref), "get broken");
    
    
    
    p.clearPref(noSuchPref);
    G_Assert(z, !p.getPref(noSuchPref, false), "clear broken");
    
    p.clearPref(testPref);
    
    G_Debug(z, "PASSED");
  }
}
#endif
