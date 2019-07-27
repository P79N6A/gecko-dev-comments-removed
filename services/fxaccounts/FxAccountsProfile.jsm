



"use strict";









this.EXPORTED_SYMBOLS = ["FxAccountsProfile"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://gre/modules/FxAccounts.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FxAccountsProfileClient",
  "resource://gre/modules/FxAccountsProfileClient.jsm");


function deepEqual(actual, expected) {
  if (actual === expected) {
    return true;
  } else if (typeof actual != "object" && typeof expected != "object") {
    return actual == expected;
  } else {
    return objEquiv(actual, expected);
  }
}

function isUndefinedOrNull(value) {
  return value === null || value === undefined;
}

function objEquiv(a, b) {
  if (isUndefinedOrNull(a) || isUndefinedOrNull(b)) {
    return false;
  }
  if (a.prototype !== b.prototype) {
    return false;
  }
  let ka, kb, key, i;
  try {
    ka = Object.keys(a);
    kb = Object.keys(b);
  } catch (e) {
    return false;
  }
  if (ka.length != kb.length) {
    return false;
  }
  ka.sort();
  kb.sort();
  for (i = ka.length - 1; i >= 0; i--) {
    key = ka[i];
    if (!deepEqual(a[key], b[key])) {
      return false;
    }
  }
  return true;
}

function hasChanged(oldData, newData) {
  return !deepEqual(oldData, newData);
}

this.FxAccountsProfile = function (options = {}) {
  this._cachedProfile = null;
  this._cachedAt = 0; 
  this._currentFetchPromise = null;
  this._isNotifying = false; 
  this.fxa = options.fxa || fxAccounts;
  this.client = options.profileClient || new FxAccountsProfileClient({
    fxa: this.fxa,
    serverURL: options.profileServerUrl,
  });

  
  
  Services.obs.addObserver(this, ON_PROFILE_CHANGE_NOTIFICATION, true);
  
  if (options.channel) {
    this.channel = options.channel;
  }
}

this.FxAccountsProfile.prototype = {
  
  
  PROFILE_FRESHNESS_THRESHOLD: 120000, 

  observe(subject, topic, data) {
    
    
    
    if (topic == ON_PROFILE_CHANGE_NOTIFICATION && !this._isNotifying) {
      log.debug("FxAccountsProfile observed profile change");
      this._cachedAt = 0;
    }
  },

  tearDown: function () {
    this.fxa = null;
    this.client = null;
    this._cachedProfile = null;
    Services.obs.removeObserver(this, ON_PROFILE_CHANGE_NOTIFICATION);
  },

  _getCachedProfile: function () {
    
    
    return Promise.resolve(this._cachedProfile);
  },

  _notifyProfileChange: function (uid) {
    this._isNotifying = true;
    Services.obs.notifyObservers(null, ON_PROFILE_CHANGE_NOTIFICATION, uid);
    this._isNotifying = false;
  },

  
  
  _cacheProfile: function (profileData) {
    if (!hasChanged(this._cachedProfile, profileData)) {
      log.debug("fetched profile matches cached copy");
      return Promise.resolve(null); 
    }
    this._cachedProfile = profileData;
    this._cachedAt = Date.now();
    return this.fxa.getSignedInUser()
      .then(userData => {
        log.debug("notifying profile changed for user ${uid}", userData);
        this._notifyProfileChange(userData.uid);
        return profileData;
      });
  },

  _fetchAndCacheProfile: function () {
    if (!this._currentFetchPromise) {
      this._currentFetchPromise = this.client.fetchProfile().then(profile => {
        return this._cacheProfile(profile).then(() => {
          return profile;
        });
      }).then(profile => {
        this._currentFetchPromise = null;
        return profile;
      }, err => {
        this._currentFetchPromise = null;
        throw err;
      });
    }
    return this._currentFetchPromise
  },

  
  
  
  getProfile: function () {
    return this._getCachedProfile()
      .then(cachedProfile => {
        if (cachedProfile) {
          if (Date.now() > this._cachedAt + this.PROFILE_FRESHNESS_THRESHOLD) {
            
            
            this._fetchAndCacheProfile().catch(err => {
              log.error("Background refresh of profile failed", err);
            });
          } else {
            log.trace("not checking freshness of profile as it remains recent");
          }
          return cachedProfile;
        }
        return this._fetchAndCacheProfile();
      })
      .then(profile => {
        return profile;
      });
  },

  QueryInterface: XPCOMUtils.generateQI([
      Ci.nsIObserver,
      Ci.nsISupportsWeakReference,
  ]),
};
