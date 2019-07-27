



"use strict";









this.EXPORTED_SYMBOLS = ["FxAccountsProfile"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");

XPCOMUtils.defineLazyModuleGetter(this, "FxAccountsProfileClient",
  "resource://gre/modules/FxAccountsProfileClient.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "FxAccountsProfileChannel",
  "resource://gre/modules/FxAccountsProfileChannel.jsm");

let fxAccountProfileChannel = null;


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

this.FxAccountsProfile = function (accountState, options = {}) {
  this.currentAccountState = accountState;
  this.client = options.profileClient || new FxAccountsProfileClient({
    serverURL: options.profileServerUrl,
    token: options.token
  });

  
  if (options.channel) {
    this.channel = options.channel;
  }
}

this.FxAccountsProfile.prototype = {

  tearDown: function () {
    this.currentAccountState = null;
    this.client = null;
  },

  _getCachedProfile: function () {
    let currentState = this.currentAccountState;
    return currentState.getUserAccountData()
      .then(cachedData => cachedData.profile);
  },

  _notifyProfileChange: function (uid) {
    Services.obs.notifyObservers(null, ON_PROFILE_CHANGE_NOTIFICATION, uid);
  },

  
  
  _cacheProfile: function (profileData) {
    let currentState = this.currentAccountState;
    if (!currentState) {
      return;
    }
    return currentState.getUserAccountData()
      .then(data => {
        if (!hasChanged(data.profile, profileData)) {
          return;
        }
        data.profile = profileData;
        return currentState.setUserAccountData(data)
          .then(() => this._notifyProfileChange(data.uid));
      });
  },

  _fetchAndCacheProfile: function () {
    return this.client.fetchProfile()
      .then(profile => {
        return this._cacheProfile(profile).then(() => profile);
      });
  },

  
  _listenForProfileChanges: function () {
    if (! fxAccountProfileChannel) {
      let contentUri = Services.urlFormatter.formatURLPref("identity.fxaccounts.settings.uri");

      fxAccountProfileChannel = new FxAccountsProfileChannel({
        content_uri: contentUri
      });
    }

    return fxAccountProfileChannel;
  },

  
  
  
  getProfile: function () {
    this._listenForProfileChanges();

    return this._getCachedProfile()
      .then(cachedProfile => {
        if (cachedProfile) {
          this._fetchAndCacheProfile();
          return cachedProfile;
        }
        return this._fetchAndCacheProfile();
      })
      .then(profile => {
        return profile;
      });
  },
};
