



const EXPORTED_SYMBOLS = ["SocialService"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "getFrameWorkerHandle", "resource://gre/modules/FrameWorker.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "WorkerAPI", "resource://gre/modules/WorkerAPI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "MozSocialAPI", "resource://gre/modules/MozSocialAPI.jsm");








let SocialServiceInternal = {
  enabled: Services.prefs.getBoolPref("social.enabled"),
  get providerArray() {
    return [p for ([, p] of Iterator(this.providers))];
  }
};

function initService() {
  
  function prefObserver(subject, topic, data) {
    SocialService._setEnabled(Services.prefs.getBoolPref("social.enabled"));
  }
  Services.prefs.addObserver("social.enabled", prefObserver, false);
  Services.obs.addObserver(function xpcomShutdown() {
    Services.obs.removeObserver(xpcomShutdown, "xpcom-shutdown");
    Services.prefs.removeObserver("social.enabled", prefObserver);
  }, "xpcom-shutdown", false);

  
  if (SocialServiceInternal.enabled)
    MozSocialAPI.enabled = true;
}

XPCOMUtils.defineLazyGetter(SocialServiceInternal, "providers", function () {
  initService();

  
  let skipLoading = false;
  try {
    skipLoading = Services.prefs.getBoolPref("social.skipLoadingProviders");
  } catch (ex) {}

  if (skipLoading)
    return {};

  
  let providers = {};
  let MANIFEST_PREFS = Services.prefs.getBranch("social.manifest.");
  let prefs = MANIFEST_PREFS.getChildList("", {});
  prefs.forEach(function (pref) {
    try {
      var manifest = JSON.parse(MANIFEST_PREFS.getCharPref(pref));
      if (manifest && typeof(manifest) == "object") {
        let provider = new SocialProvider(manifest, SocialServiceInternal.enabled);
        providers[provider.origin] = provider;
      }
    } catch (err) {
      Cu.reportError("SocialService: failed to load provider: " + pref +
                     ", exception: " + err);
    }
  });

  return providers;
});

function schedule(callback) {
  Services.tm.mainThread.dispatch(callback, Ci.nsIThread.DISPATCH_NORMAL);
}


const SocialService = {
  get enabled() {
    return SocialServiceInternal.enabled;
  },
  set enabled(val) {
    let enable = !!val;
    if (enable == SocialServiceInternal.enabled)
      return;

    Services.prefs.setBoolPref("social.enabled", enable);
    this._setEnabled(enable);
  },
  _setEnabled: function _setEnabled(enable) {
    SocialServiceInternal.providerArray.forEach(function (p) p.enabled = enable);
    SocialServiceInternal.enabled = enable;
    MozSocialAPI.enabled = enable;
    Services.obs.notifyObservers(null, "social:pref-changed", enable ? "enabled" : "disabled");
  },

  
  addProvider: function addProvider(manifest, onDone) {
    if (SocialServiceInternal.providers[manifest.origin])
      throw new Error("SocialService.addProvider: provider with this origin already exists");

    let provider = new SocialProvider(manifest, SocialServiceInternal.enabled);
    SocialServiceInternal.providers[provider.origin] = provider;

    schedule(function () {
      onDone(provider);
    });
  },

  
  
  removeProvider: function removeProvider(origin, onDone) {
    if (!(origin in SocialServiceInternal.providers))
      throw new Error("SocialService.removeProvider: no provider with this origin exists!");

    let provider = SocialServiceInternal.providers[origin];
    provider.enabled = false;

    delete SocialServiceInternal.providers[origin];

    if (onDone)
      schedule(onDone);
  },

  
  getProvider: function getProvider(origin, onDone) {
    schedule((function () {
      onDone(SocialServiceInternal.providers[origin] || null);
    }).bind(this));
  },

  
  getProviderList: function getProviderList(onDone) {
    schedule(function () {
      onDone(SocialServiceInternal.providerArray);
    });
  }
};









function SocialProvider(input, enabled) {
  if (!input.name)
    throw new Error("SocialProvider must be passed a name");
  if (!input.origin)
    throw new Error("SocialProvider must be passed an origin");

  this.name = input.name;
  this.iconURL = input.iconURL;
  this.workerURL = input.workerURL;
  this.sidebarURL = input.sidebarURL;
  this.origin = input.origin;
  this.ambientNotificationIcons = {};

  
  this._enabled = !(enabled == false);
  if (this._enabled)
    this._activate();
}

SocialProvider.prototype = {
  
  
  _enabled: true,
  get enabled() {
    return this._enabled;
  },
  set enabled(val) {
    let enable = !!val;
    if (enable == this._enabled)
      return;

    this._enabled = enable;

    if (enable) {
      this._activate();
    } else {
      this._terminate();
    }
  },

  
  
  port: null,

  
  
  workerAPI: null,

  
  
  
  
  
  profile: null,

  
  
  
  
  ambientNotificationIcons: null,

  
  updateUserProfile: function(profile) {
    this.profile = profile;

    
    if (profile.portrait) {
      try {
        let portraitUri = Services.io.newURI(profile.portrait, null, null);

        let scheme = portraitUri ? portraitUri.scheme : "";
        if (scheme != "data" && scheme != "http" && scheme != "https") {
          profile.portrait = "";
        }
      } catch (ex) {
        profile.portrait = "";
      }
    }

    if (profile.iconURL)
      this.iconURL = profile.iconURL;

    if (!profile.displayName)
      profile.displayName = profile.userName;

    
    
    
    if (!profile.userName) {
      this.profile = {};
      this.ambientNotificationIcons = {};
      Services.obs.notifyObservers(null, "social:ambient-notification-changed", this.origin);
    }

    Services.obs.notifyObservers(null, "social:profile-changed", this.origin);
  },

  
  setAmbientNotification: function(notification) {
    if (!this.profile.userName)
      throw new Error("unable to set notifications while logged out");
    this.ambientNotificationIcons[notification.name] = notification;

    Services.obs.notifyObservers(null, "social:ambient-notification-changed", this.origin);
  },

  
  _activate: function _activate() {
    
    
    let workerAPIPort = this._getWorkerPort();
    if (workerAPIPort)
      this.workerAPI = new WorkerAPI(this, workerAPIPort);

    this.port = this._getWorkerPort();
  },

  _terminate: function _terminate() {
    if (this.workerURL) {
      try {
        getFrameWorkerHandle(this.workerURL, null).terminate();
      } catch (e) {
        Cu.reportError("SocialProvider FrameWorker termination failed: " + e);
      }
    }
    this.port = null;
    this.workerAPI = null;
  },

  







  _getWorkerPort: function _getWorkerPort(window) {
    if (!this.workerURL || !this.enabled)
      return null;
    try {
      return getFrameWorkerHandle(this.workerURL, window).port;
    } catch (ex) {
      Cu.reportError("SocialProvider: retrieving worker port failed:" + ex);
      return null;
    }
  }
}
