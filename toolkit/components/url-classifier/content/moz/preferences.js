# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Google Safe Browsing.
#
# The Initial Developer of the Original Code is Google Inc.
# Portions created by the Initial Developer are Copyright (C) 2006
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Fritz Schneider <fritz@google.com> (original author)
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****













































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
  var observer = new G_PreferenceObserver(callback);
  
  if (!this.observers_[which])
    this.observers_[which] = new G_ObjectSafeMap();
  this.observers_[which].insert(callback, observer);
  this.prefs_.addObserver(which, observer, false );
}







G_Preferences.prototype.removeObserver = function(which, callback) {
  var observer = this.observers_[which].find(callback);
  G_Assert(this, !!observer, "Tried to unregister a nonexistant observer"); 
  this.prefs_.removeObserver(which, observer);
  this.observers_[which].erase(callback);
}









function G_PreferenceObserver(callback) {
  this.debugZone = "prefobserver";
  this.callback_ = callback;
}











G_PreferenceObserver.prototype.observe = function(subject, topic, data) {
  G_Debug(this, "Observed pref change: " + data);
  this.callback_(data);
}






G_PreferenceObserver.prototype.QueryInterface = function(iid) {
  var Ci = Ci;
  if (iid.equals(Ci.nsISupports) || 
      iid.equals(Ci.nsIObserver) ||
      iid.equals(Ci.nsISupportsWeakReference))
    return this;
  throw Components.results.NS_ERROR_NO_INTERFACE;
}

#ifdef DEBUG

function TEST_G_Preferences() {
  if (G_GDEBUG) {
    var z = "preferences UNITTEST";
    G_debugService.enableZone(z);
    G_Debug(z, "Starting");

    var p = new G_Preferences();
    
    var testPref = "test-preferences-unittest";
    var noSuchPref = "test-preferences-unittest-aypabtu";
    
    
    var observeCount = 0;
    function observe(prefChanged) {
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
