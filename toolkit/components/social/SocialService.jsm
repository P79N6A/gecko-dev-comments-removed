



this.EXPORTED_SYMBOLS = ["SocialService"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "getFrameWorkerHandle", "resource://gre/modules/FrameWorker.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "WorkerAPI", "resource://gre/modules/WorkerAPI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "MozSocialAPI", "resource://gre/modules/MozSocialAPI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "DeferredTask", "resource://gre/modules/DeferredTask.jsm");








let SocialServiceInternal = {
  enabled: Services.prefs.getBoolPref("social.enabled"),
  get providerArray() {
    return [p for ([, p] of Iterator(this.providers))];
  },
  get manifests() {
    
    let MANIFEST_PREFS = Services.prefs.getBranch("social.manifest.");
    let prefs = MANIFEST_PREFS.getChildList("", []);
    for (let pref of prefs) {
      try {
        var manifest = JSON.parse(MANIFEST_PREFS.getCharPref(pref));
        if (manifest && typeof(manifest) == "object" && manifest.origin)
          yield manifest;
      } catch (err) {
        Cu.reportError("SocialService: failed to load manifest: " + pref +
                       ", exception: " + err);
      }
    }
  }
};

let ActiveProviders = {
  get _providers() {
    delete this._providers;
    this._providers = {};
    try {
      let pref = Services.prefs.getComplexValue("social.activeProviders",
                                                Ci.nsISupportsString);
      this._providers = JSON.parse(pref);
    } catch(ex) {}
    return this._providers;
  },

  has: function (origin) {
    return (origin in this._providers);
  },

  add: function (origin) {
    this._providers[origin] = 1;
    this._deferredTask.start();
  },

  delete: function (origin) {
    delete this._providers[origin];
    this._deferredTask.start();
  },

  flush: function () {
    this._deferredTask.flush();
  },

  get _deferredTask() {
    delete this._deferredTask;
    return this._deferredTask = new DeferredTask(this._persist.bind(this), 0);
  },

  _persist: function () {
    let string = Cc["@mozilla.org/supports-string;1"].
                 createInstance(Ci.nsISupportsString);
    string.data = JSON.stringify(this._providers);
    Services.prefs.setComplexValue("social.activeProviders",
                                   Ci.nsISupportsString, string);
  }
};

function migrateSettings() {
  try {
    
    Services.prefs.getCharPref("social.activeProviders");
    return;
  } catch(e) {
    try {
      let active = Services.prefs.getBoolPref("social.active");
      if (active) {
        for (let manifest of SocialServiceInternal.manifests) {
          ActiveProviders.add(manifest.origin);
          return;
        }
      }
    } catch(e) {
      
    }
  }
}

function initService() {
  Services.obs.addObserver(function xpcomShutdown() {
    ActiveProviders.flush();
    SocialService._providerListeners = null;
    Services.obs.removeObserver(xpcomShutdown, "xpcom-shutdown");
  }, "xpcom-shutdown", false);

  migrateSettings();
  
  if (SocialServiceInternal.enabled)
    MozSocialAPI.enabled = true;
}

XPCOMUtils.defineLazyGetter(SocialServiceInternal, "providers", function () {
  initService();
  let providers = {};
  for (let manifest of this.manifests) {
    try {
      if (ActiveProviders.has(manifest.origin)) {
        let provider = new SocialProvider(manifest);
        providers[provider.origin] = provider;
      }
    } catch (err) {
      Cu.reportError("SocialService: failed to load provider: " + manifest.origin +
                     ", exception: " + err);
    }
  }
  return providers;
});

function schedule(callback) {
  Services.tm.mainThread.dispatch(callback, Ci.nsIThread.DISPATCH_NORMAL);
}


this.SocialService = {
  get enabled() {
    return SocialServiceInternal.enabled;
  },
  set enabled(val) {
    let enable = !!val;

    
    
    if (enable == SocialServiceInternal.enabled &&
        !Services.appinfo.inSafeMode)
      return;

    
    if (!enable)
      SocialServiceInternal.providerArray.forEach(function (p) p.enabled = false);

    SocialServiceInternal.enabled = enable;
    MozSocialAPI.enabled = enable;
    Services.obs.notifyObservers(null, "social:pref-changed", enable ? "enabled" : "disabled");
    Services.telemetry.getHistogramById("SOCIAL_TOGGLED").add(enable);
  },

  
  
  
  addBuiltinProvider: function addBuiltinProvider(origin, onDone) {
    if (SocialServiceInternal.providers[origin]) {
      schedule(function() {
        onDone(SocialServiceInternal.providers[origin]);
      });
      return;
    }
    for (let manifest of SocialServiceInternal.manifests) {
      if (manifest.origin == origin) {
        this.addProvider(manifest, onDone);
        return;
      }
    }
    schedule(function() {
      onDone(null);
    });
  },

  
  addProvider: function addProvider(manifest, onDone) {
    if (SocialServiceInternal.providers[manifest.origin])
      throw new Error("SocialService.addProvider: provider with this origin already exists");

    let provider = new SocialProvider(manifest);
    SocialServiceInternal.providers[provider.origin] = provider;
    ActiveProviders.add(provider.origin);

    schedule(function () {
      this._notifyProviderListeners("provider-added",
                                    SocialServiceInternal.providerArray);
      if (onDone)
        onDone(provider);
    }.bind(this));
  },

  
  
  removeProvider: function removeProvider(origin, onDone) {
    if (!(origin in SocialServiceInternal.providers))
      throw new Error("SocialService.removeProvider: no provider with this origin exists!");

    let provider = SocialServiceInternal.providers[origin];
    provider.enabled = false;

    ActiveProviders.delete(provider.origin);

    delete SocialServiceInternal.providers[origin];

    schedule(function () {
      this._notifyProviderListeners("provider-removed",
                                    SocialServiceInternal.providerArray);
      if (onDone)
        onDone();
    }.bind(this));
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
  },

  canActivateOrigin: function canActivateOrigin(origin) {
    for (let manifest in SocialServiceInternal.manifests) {
      if (manifest.origin == origin)
        return true;
    }
    return false;
  },

  _providerListeners: new Map(),
  registerProviderListener: function registerProviderListener(listener) {
    this._providerListeners.set(listener, 1);
  },
  unregisterProviderListener: function unregisterProviderListener(listener) {
    this._providerListeners.delete(listener);
  },

  _notifyProviderListeners: function (topic, data) {
    for (let [listener, ] of this._providerListeners) {
      try {
        listener(topic, data);
      } catch (ex) {
        Components.utils.reportError("SocialService: provider listener threw an exception: " + ex);
      }
    }
  }
};








function SocialProvider(input) {
  if (!input.name)
    throw new Error("SocialProvider must be passed a name");
  if (!input.origin)
    throw new Error("SocialProvider must be passed an origin");

  this.name = input.name;
  this.iconURL = input.iconURL;
  this.icon32URL = input.icon32URL;
  this.icon64URL = input.icon64URL;
  this.workerURL = input.workerURL;
  this.sidebarURL = input.sidebarURL;
  this.origin = input.origin;
  let originUri = Services.io.newURI(input.origin, null, null);
  this.principal = Services.scriptSecurityManager.getNoAppCodebasePrincipal(originUri);
  this.ambientNotificationIcons = {};
  this.errorState = null;
}

SocialProvider.prototype = {
  
  
  _enabled: false,
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

  
  
  workerAPI: null,

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  profile: undefined,

  
  
  
  
  _recommendInfo: null,
  get recommendInfo() {
    return this._recommendInfo;
  },
  set recommendInfo(data) {
    
    
    let promptImages = {};
    let promptMessages = {};
    function reportError(reason) {
      Cu.reportError("Invalid recommend data from provider: " + reason + ": sharing is disabled for this provider");
      
      
      this._recommendInfo = null;
      Services.obs.notifyObservers(null, "social:recommend-info-changed", this.origin);
    }
    if (!data ||
        !data.images || typeof data.images != "object" ||
        !data.messages || typeof data.messages != "object") {
      reportError("data is missing valid 'images' or 'messages' elements");
      return;
    }
    for (let sub of ["share", "unshare"]) {
      let url = data.images[sub];
      if (!url || typeof url != "string" || url.length == 0) {
        reportError('images["' + sub + '"] is missing or not a non-empty string');
        return;
      }
      
      
      
      
      let imgUri = this.resolveUri(url);
      if (!imgUri) {
        reportError('images["' + sub + '"] is an invalid URL');
        return;
      }
      promptImages[sub] = imgUri.spec;
    }
    for (let sub of ["shareTooltip", "unshareTooltip",
                     "sharedLabel", "unsharedLabel", "unshareLabel",
                     "portraitLabel",
                     "unshareConfirmLabel", "unshareConfirmAccessKey",
                     "unshareCancelLabel", "unshareCancelAccessKey"]) {
      if (typeof data.messages[sub] != "string" || data.messages[sub].length == 0) {
        reportError('messages["' + sub + '"] is not a valid string');
        return;
      }
      promptMessages[sub] = data.messages[sub];
    }
    this._recommendInfo = {images: promptImages, messages: promptMessages};
    Services.obs.notifyObservers(null, "social:recommend-info-changed", this.origin);
  },

  
  
  
  
  ambientNotificationIcons: null,

  
  updateUserProfile: function(profile) {
    if (!profile)
      profile = {};
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
    
    
    let workerAPIPort = this.getWorkerPort();
    if (workerAPIPort)
      this.workerAPI = new WorkerAPI(this, workerAPIPort);
  },

  _terminate: function _terminate() {
    if (this.workerURL) {
      try {
        getFrameWorkerHandle(this.workerURL).terminate();
      } catch (e) {
        Cu.reportError("SocialProvider FrameWorker termination failed: " + e);
      }
    }
    if (this.workerAPI) {
      this.workerAPI.terminate();
    }
    this.errorState = null;
    this.workerAPI = null;
    this.profile = undefined;
  },

  







  getWorkerPort: function getWorkerPort(window) {
    if (!this.workerURL || !this.enabled)
      return null;
    return getFrameWorkerHandle(this.workerURL, window,
                                "SocialProvider:" + this.origin, this.origin).port;
  },

  






  isSameOrigin: function isSameOrigin(uri, allowIfInheritsPrincipal) {
    if (!uri)
      return false;
    if (typeof uri == "string") {
      try {
        uri = Services.io.newURI(uri, null, null);
      } catch (ex) {
        
        return false;
      }
    }
    try {
      this.principal.checkMayLoad(
        uri, 
        false, 
        allowIfInheritsPrincipal
      );
      return true;
    } catch (ex) {
      return false;
    }
  },

  






  resolveUri: function resolveUri(url) {
    try {
      let fullURL = this.principal.URI.resolve(url);
      return Services.io.newURI(fullURL, null, null);
    } catch (ex) {
      Cu.reportError("mozSocial: failed to resolve window URL: " + url + "; " + ex);
      return null;
    }
  }
}
