



this.EXPORTED_SYMBOLS = ["SocialService"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/AddonManager.jsm");

const URI_EXTENSION_STRINGS  = "chrome://mozapps/locale/extensions/extensions.properties";
const ADDON_TYPE_SERVICE     = "service";
const ID_SUFFIX              = "@services.mozilla.org";
const STRING_TYPE_NAME       = "type.%ID%.name";

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
      
      if (!MANIFEST_PREFS.prefHasUserValue(pref))
        continue;
      try {
        var manifest = JSON.parse(MANIFEST_PREFS.getComplexValue(pref, Ci.nsISupportsString).data);
        if (manifest && typeof(manifest) == "object" && manifest.origin)
          yield manifest;
      } catch (err) {
        Cu.reportError("SocialService: failed to load manifest: " + pref +
                       ", exception: " + err);
      }
    }
  },
  getManifestByOrigin: function(origin) {
    for (let manifest of SocialServiceInternal.manifests) {
      if (origin == manifest.origin) {
        return manifest;
      }
    }
    return null;
  },
  getManifestPrefname: function(origin) {
    
    
    let MANIFEST_PREFS = Services.prefs.getBranch("social.manifest.");
    let prefs = MANIFEST_PREFS.getChildList("", []);
    for (let pref of prefs) {
      try {
        var manifest = JSON.parse(MANIFEST_PREFS.getComplexValue(pref, Ci.nsISupportsString).data);
        if (manifest.origin == origin) {
          return pref;
        }
      } catch (err) {
        Cu.reportError("SocialService: failed to load manifest: " + pref +
                       ", exception: " + err);
      }
    }
    let originUri = Services.io.newURI(origin, null, null);
    return originUri.hostPort.replace('.','-');
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
  let activeProviders;
  try {
    activeProviders = Services.prefs.getCharPref("social.activeProviders");
  } catch(e) {
    
  }
  if (activeProviders) {
    
    
    for (let origin in ActiveProviders._providers) {
      let prefname;
      let manifest;
      let defaultManifest;
      try {
        prefname = getPrefnameFromOrigin(origin);
        manifest = JSON.parse(Services.prefs.getComplexValue(prefname, Ci.nsISupportsString).data);
      } catch(e) {
        
        
        
        
        ActiveProviders.delete(origin);
        ActiveProviders.flush();
        continue;
      }
      let needsUpdate = !manifest.updateDate;
      
      try {
        defaultManifest = Services.prefs.getDefaultBranch(null)
                        .getComplexValue(prefname, Ci.nsISupportsString).data;
        defaultManifest = JSON.parse(defaultManifest);
        if (defaultManifest.shareURL && !manifest.shareURL) {
          manifest.shareURL = defaultManifest.shareURL;
          needsUpdate = true;
        }
      } catch(e) {
        
      }
      if (needsUpdate) {
        
        
        delete manifest.builtin;
        
        manifest.updateDate = Date.now();
        if (!manifest.installDate)
          manifest.installDate = 0; 

        let string = Cc["@mozilla.org/supports-string;1"].
                     createInstance(Ci.nsISupportsString);
        string.data = JSON.stringify(manifest);
        Services.prefs.setComplexValue(prefname, Ci.nsISupportsString, string);
      }
    }
    return;
  }

  
  let active;
  try {
    active = Services.prefs.getBoolPref("social.active");
  } catch(e) {}
  if (!active)
    return;

  
  
  let manifestPrefs = Services.prefs.getDefaultBranch("social.manifest.");
  let prefs = manifestPrefs.getChildList("", []);
  for (let pref of prefs) {
    try {
      let manifest;
      try {
        manifest = JSON.parse(manifestPrefs.getComplexValue(pref, Ci.nsISupportsString).data);
      } catch(e) {
        
        continue;
      }
      if (manifest && typeof(manifest) == "object" && manifest.origin) {
        
        
        delete manifest.builtin;
        if (!manifest.updateDate) {
          manifest.updateDate = Date.now();
          manifest.installDate = 0; 
        }

        let string = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
        string.data = JSON.stringify(manifest);
        
        Services.prefs.setComplexValue("social.manifest." + pref, Ci.nsISupportsString, string);
        ActiveProviders.add(manifest.origin);
        ActiveProviders.flush();
        
        
        return;
      }
    } catch (err) {
      Cu.reportError("SocialService: failed to load manifest: " + pref + ", exception: " + err);
    }
  }
}

function initService() {
  Services.obs.addObserver(function xpcomShutdown() {
    ActiveProviders.flush();
    SocialService._providerListeners = null;
    Services.obs.removeObserver(xpcomShutdown, "xpcom-shutdown");
  }, "xpcom-shutdown", false);

  try {
    migrateSettings();
  } catch(e) {
    
    
    
    Cu.reportError("Error migrating social settings: " + e);
  }
  
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
    let manifest = SocialServiceInternal.getManifestByOrigin(origin);
    if (manifest) {
      let addon = new AddonWrapper(manifest);
      AddonManagerPrivate.callAddonListeners("onEnabling", addon, false);
      addon.pendingOperations |= AddonManager.PENDING_ENABLE;
      this.addProvider(manifest, onDone);
      addon.pendingOperations -= AddonManager.PENDING_ENABLE;
      AddonManagerPrivate.callAddonListeners("onEnabled", addon);
      return;
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
      throw new Error("SocialService.removeProvider: no provider with origin " + origin + " exists!");

    let provider = SocialServiceInternal.providers[origin];
    let manifest = SocialServiceInternal.getManifestByOrigin(origin);
    let addon = manifest && new AddonWrapper(manifest);
    if (addon) {
      AddonManagerPrivate.callAddonListeners("onDisabling", addon, false);
      addon.pendingOperations |= AddonManager.PENDING_DISABLE;
    }
    provider.enabled = false;

    ActiveProviders.delete(provider.origin);

    delete SocialServiceInternal.providers[origin];

    if (addon) {
      
      
      addon.pendingOperations -= AddonManager.PENDING_DISABLE;
      AddonManagerPrivate.callAddonListeners("onDisabled", addon);
      AddonManagerPrivate.notifyAddonChanged(addon.id, ADDON_TYPE_SERVICE, false);
    }

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

  getOriginActivationType: function(origin) {
    let prefname = SocialServiceInternal.getManifestPrefname(origin);
    if (Services.prefs.getDefaultBranch("social.manifest.").getPrefType(prefname) == Services.prefs.PREF_STRING)
      return 'builtin';

    let whitelist = Services.prefs.getCharPref("social.whitelist").split(',');
    if (whitelist.indexOf(origin) >= 0)
      return 'whitelist';

    let directories = Services.prefs.getCharPref("social.directories").split(',');
    if (directories.indexOf(origin) >= 0)
      return 'directory';

    return 'foreign';
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
  },

  _manifestFromData: function(type, data, principal) {
    let sameOriginRequired = ['workerURL', 'sidebarURL', 'shareURL'];

    if (type == 'directory') {
      
      if (!data['origin']) {
        Cu.reportError("SocialService.manifestFromData directory service provided manifest without origin.");
        return null;
      }
      let URI = Services.io.newURI(data.origin, null, null);
      principal = Services.scriptSecurityManager.getNoAppCodebasePrincipal(URI);
    }
    
    data.origin = principal.origin;

    
    
    
    
    if (!data['workerURL'] && !data['sidebarURL'] && !data['shareURL']) {
      Cu.reportError("SocialService.manifestFromData manifest missing required workerURL or sidebarURL.");
      return null;
    }
    if (!data['name'] || !data['iconURL']) {
      Cu.reportError("SocialService.manifestFromData manifest missing name or iconURL.");
      return null;
    }
    for (let url of sameOriginRequired) {
      if (data[url]) {
        try {
          data[url] = Services.io.newURI(principal.URI.resolve(data[url]), null, null).spec;
        } catch(e) {
          Cu.reportError("SocialService.manifestFromData same-origin missmatch in manifest for " + principal.origin);
          return null;
        }
      }
    }
    return data;
  },

  _getChromeWindow: function(aWindow) {
    var chromeWin = aWindow
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIWebNavigation)
      .QueryInterface(Ci.nsIDocShellTreeItem)
      .rootTreeItem
      .QueryInterface(Ci.nsIInterfaceRequestor)
      .getInterface(Ci.nsIDOMWindow)
      .QueryInterface(Ci.nsIDOMChromeWindow);
    return chromeWin;
  },

  _showInstallNotification: function(aDOMDocument, aAddonInstaller) {
    let brandBundle = Services.strings.createBundle("chrome://branding/locale/brand.properties");
    let browserBundle = Services.strings.createBundle("chrome://browser/locale/browser.properties");
    let requestingWindow = aDOMDocument.defaultView.top;
    let chromeWin = this._getChromeWindow(requestingWindow).wrappedJSObject;
    let browser = chromeWin.gBrowser.getBrowserForDocument(aDOMDocument);
    let requestingURI =  Services.io.newURI(aDOMDocument.location.href, null, null);

    let productName = brandBundle.GetStringFromName("brandShortName");

    let message = browserBundle.formatStringFromName("service.install.description",
                                                     [requestingURI.host, productName], 2);

    let action = {
      label: browserBundle.GetStringFromName("service.install.ok.label"),
      accessKey: browserBundle.GetStringFromName("service.install.ok.accesskey"),
      callback: function() {
        aAddonInstaller.install();
      },
    };

    let link = chromeWin.document.getElementById("servicesInstall-learnmore-link");
    link.value = browserBundle.GetStringFromName("service.install.learnmore");
    link.href = Services.urlFormatter.formatURLPref("app.support.baseURL") + "social-api";

    let anchor = "servicesInstall-notification-icon";
    let notificationid = "servicesInstall";
    chromeWin.PopupNotifications.show(browser, notificationid, message, anchor,
                                      action, [], {});
  },

  installProvider: function(aDOMDocument, data, installCallback) {
    let sourceURI = aDOMDocument.location.href;
    let installOrigin = aDOMDocument.nodePrincipal.origin;

    let id = getAddonIDFromOrigin(installOrigin);
    let version = data && data.version ? data.version : "0";
    if (Services.blocklist.getAddonBlocklistState(id, version) == Ci.nsIBlocklistService.STATE_BLOCKED)
      throw new Error("installProvider: provider with origin [" +
                      installOrigin + "] is blocklisted");

    let installType = this.getOriginActivationType(installOrigin);
    let manifest;
    if (data) {
      
      manifest = this._manifestFromData(installType, data, aDOMDocument.nodePrincipal);
      if (!manifest)
        throw new Error("SocialService.installProvider: service configuration is invalid from " + sourceURI);
    }
    let installer;
    switch(installType) {
      case "foreign":
        if (!Services.prefs.getBoolPref("social.remote-install.enabled"))
          throw new Error("Remote install of services is disabled");
        if (!manifest)
          throw new Error("Cannot install provider without manifest data");

        installer = new AddonInstaller(sourceURI, manifest, installCallback);
        this._showInstallNotification(aDOMDocument, installer);
        break;
      case "builtin":
        
        
        
        
        
        
        
        if (!manifest) {
          let prefname = getPrefnameFromOrigin(installOrigin);
          manifest = Services.prefs.getDefaultBranch(null)
                          .getComplexValue(prefname, Ci.nsISupportsString).data;
          manifest = JSON.parse(manifest);
          
          if (manifest.builtin)
            delete manifest.builtin;
        }
      case "directory":
        
      case "whitelist":
        
        if (!manifest)
          throw new Error("Cannot install provider without manifest data");
        installer = new AddonInstaller(sourceURI, manifest, installCallback);
        installer.install();
        break;
      default:
        throw new Error("SocialService.installProvider: Invalid install type "+installType+"\n");
        break;
    }
  },

  uninstallProvider: function(origin) {
    let manifest = SocialServiceInternal.getManifestByOrigin(origin);
    let addon = new AddonWrapper(manifest);
    addon.uninstall();
  }
};








function SocialProvider(input) {
  if (!input.name)
    throw new Error("SocialProvider must be passed a name");
  if (!input.origin)
    throw new Error("SocialProvider must be passed an origin");

  let id = getAddonIDFromOrigin(input.origin);
  if (Services.blocklist.getAddonBlocklistState(id, input.version || "0") == Ci.nsIBlocklistService.STATE_BLOCKED)
    throw new Error("SocialProvider: provider with origin [" +
                    input.origin + "] is blocklisted");

  this.name = input.name;
  this.iconURL = input.iconURL;
  this.icon32URL = input.icon32URL;
  this.icon64URL = input.icon64URL;
  this.workerURL = input.workerURL;
  this.sidebarURL = input.sidebarURL;
  this.shareURL = input.shareURL;
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

  
  
  
  
  _pageMarkInfo: null,
  get pageMarkInfo() {
    return this._pageMarkInfo;
  },
  set pageMarkInfo(data) {
    
    let promptImages = {};
    let promptMessages = {};
    function reportError(reason) {
      Cu.reportError("Invalid page-mark data from provider: " + reason + ": marking is disabled for this provider");
      
      
      this._pageMarkInfo = null;
      Services.obs.notifyObservers(null, "social:page-mark-config", this.origin);
    }
    if (!data ||
        !data.images || typeof data.images != "object" ||
        !data.messages || typeof data.messages != "object") {
      reportError("data is missing valid 'images' or 'messages' elements");
      return;
    }
    for (let sub of ["marked", "unmarked"]) {
      let url = data.images[sub];
      if (!url || typeof url != "string" || url.length == 0) {
        reportError('images["' + sub + '"] is not a valid string');
        return;
      }
      
      
      
      
      let imgUri = this.resolveUri(url);
      if (!imgUri) {
        reportError('images["' + sub + '"] is an invalid URL');
        return;
      }
      promptImages[sub] = imgUri.spec;
    }
    for (let sub of ["markedTooltip", "unmarkedTooltip", "markedLabel", "unmarkedLabel"]) {
      if (typeof data.messages[sub] != "string" || data.messages[sub].length == 0) {
        reportError('messages["' + sub + '"] is not a valid string');
        return;
      }
      promptMessages[sub] = data.messages[sub];
    }
    this._pageMarkInfo = {images: promptImages, messages: promptMessages};
    Services.obs.notifyObservers(null, "social:page-mark-config", this.origin);
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

function getAddonIDFromOrigin(origin) {
  let originUri = Services.io.newURI(origin, null, null);
  return originUri.host + ID_SUFFIX;
}

function getPrefnameFromOrigin(origin) {
  return "social.manifest." + SocialServiceInternal.getManifestPrefname(origin);
}

function AddonInstaller(sourceURI, aManifest, installCallback) {
  aManifest.updateDate = Date.now();
  
  let manifest = SocialServiceInternal.getManifestByOrigin(aManifest.origin);
  if (manifest && manifest.installDate)
    aManifest.installDate = manifest.installDate;
  else
    aManifest.installDate = aManifest.updateDate;

  this.sourceURI = sourceURI;
  this.install = function() {
    let addon = this.addon;
    AddonManagerPrivate.callInstallListeners("onExternalInstall", null, addon, null, false);
    AddonManagerPrivate.callAddonListeners("onInstalling", addon, false);

    let string = Cc["@mozilla.org/supports-string;1"].
                 createInstance(Ci.nsISupportsString);
    string.data = JSON.stringify(aManifest);
    Services.prefs.setComplexValue(getPrefnameFromOrigin(aManifest.origin), Ci.nsISupportsString, string);

    AddonManagerPrivate.callAddonListeners("onInstalled", addon);
    installCallback(aManifest);
  };
  this.cancel = function() {
    Services.prefs.clearUserPref(getPrefnameFromOrigin(aManifest.origin))
  },
  this.addon = new AddonWrapper(aManifest);
};

var SocialAddonProvider = {
  startup: function() {},

  shutdown: function() {},

  updateAddonAppDisabledStates: function() {
    
    for (let manifest of SocialServiceInternal.manifests) {
      try {
        if (ActiveProviders.has(manifest.origin)) {
          let id = getAddonIDFromOrigin(manifest.origin);
          if (Services.blocklist.getAddonBlocklistState(id, manifest.version || "0") != Ci.nsIBlocklistService.STATE_NOT_BLOCKED) {
            SocialService.removeProvider(manifest.origin);
          }
        }
      } catch(e) {
        Cu.reportError(e);
      }
    }
  },

  getAddonByID: function(aId, aCallback) {
    for (let manifest of SocialServiceInternal.manifests) {
      if (aId == getAddonIDFromOrigin(manifest.origin)) {
        aCallback(new AddonWrapper(manifest));
        return;
      }
    }
    aCallback(null);
  },

  getAddonsByTypes: function(aTypes, aCallback) {
    if (aTypes && aTypes.indexOf(ADDON_TYPE_SERVICE) == -1) {
      aCallback([]);
      return;
    }
    aCallback([new AddonWrapper(a) for each (a in SocialServiceInternal.manifests)]);
  },

  removeAddon: function(aAddon) {
    AddonManagerPrivate.callAddonListeners("onUninstalling", aAddon, false);
    aAddon.pendingOperations |= AddonManager.PENDING_UNINSTALL;
    Services.prefs.clearUserPref(getPrefnameFromOrigin(aAddon.manifest.origin));
    aAddon.pendingOperations -= AddonManager.PENDING_UNINSTALL;
    AddonManagerPrivate.callAddonListeners("onUninstalled", aAddon);
  }
}


function AddonWrapper(aManifest) {
  this.manifest = aManifest;
  this.id = getAddonIDFromOrigin(this.manifest.origin);
  this._pending = AddonManager.PENDING_NONE;
}
AddonWrapper.prototype = {
  get type() {
    return ADDON_TYPE_SERVICE;
  },

  get appDisabled() {
    return this.blocklistState == Ci.nsIBlocklistService.STATE_BLOCKED;
  },

  set softDisabled(val) {
    this.userDisabled = val;
  },

  get softDisabled() {
    return this.userDisabled;
  },

  get isCompatible() {
    return true;
  },

  get isPlatformCompatible() {
    return true;
  },

  get scope() {
    return AddonManager.SCOPE_PROFILE;
  },

  get foreignInstall() {
    return false;
  },

  isCompatibleWith: function(appVersion, platformVersion) {
    return true;
  },

  get providesUpdatesSecurely() {
    return true;
  },

  get blocklistState() {
    return Services.blocklist.getAddonBlocklistState(this.id, this.version || "0");
  },

  get blocklistURL() {
    return Services.blocklist.getAddonBlocklistURL(this.id, this.version || "0");
  },

  get screenshots() {
    return [];
  },

  get pendingOperations() {
    return this._pending || AddonManager.PENDING_NONE;
  },
  set pendingOperations(val) {
    this._pending = val;
  },

  get operationsRequiringRestart() {
    return AddonManager.OP_NEEDS_RESTART_NONE;
  },

  get size() {
    return null;
  },

  get permissions() {
    let permissions = 0;
    
    if (Services.prefs.prefHasUserValue(getPrefnameFromOrigin(this.manifest.origin)))
      permissions = AddonManager.PERM_CAN_UNINSTALL;
    if (!this.appDisabled) {
      if (this.userDisabled) {
        permissions |= AddonManager.PERM_CAN_ENABLE;
      } else {
        permissions |= AddonManager.PERM_CAN_DISABLE;
      }
    }
    return permissions;
  },

  findUpdates: function(listener, reason, appVersion, platformVersion) {
    if ("onNoCompatibilityUpdateAvailable" in listener)
      listener.onNoCompatibilityUpdateAvailable(this);
    if ("onNoUpdateAvailable" in listener)
      listener.onNoUpdateAvailable(this);
    if ("onUpdateFinished" in listener)
      listener.onUpdateFinished(this);
  },

  get isActive() {
    return ActiveProviders.has(this.manifest.origin);
  },

  get name() {
    return this.manifest.name;
  },
  get version() {
    return this.manifest.version ? this.manifest.version : "";
  },

  get iconURL() {
    return this.manifest.icon32URL ? this.manifest.icon32URL : this.manifest.iconURL;
  },
  get icon64URL() {
    return this.manifest.icon64URL;
  },
  get icons() {
    let icons = {
      16: this.manifest.iconURL
    };
    if (this.manifest.icon32URL)
      icons[32] = this.manifest.icon32URL;
    if (this.manifest.icon64URL)
      icons[64] = this.manifest.icon64URL;
    return icons;
  },

  get description() {
    return this.manifest.description;
  },
  get homepageURL() {
    return this.manifest.homepageURL;
  },
  get defaultLocale() {
    return this.manifest.defaultLocale;
  },
  get selectedLocale() {
    return this.manifest.selectedLocale;
  },

  get installDate() {
    return this.manifest.installDate ? new Date(this.manifest.installDate) : null;
  },
  get updateDate() {
    return this.manifest.updateDate ? new Date(this.manifest.updateDate) : null;
  },

  get creator() {
    return new AddonManagerPrivate.AddonAuthor(this.manifest.author);
  },

  get userDisabled() {
    return this.appDisabled || !ActiveProviders.has(this.manifest.origin);
  },

  set userDisabled(val) {
    if (val == this.userDisabled)
      return val;
    if (val) {
      SocialService.removeProvider(this.manifest.origin);
    } else if (!this.appDisabled) {
      SocialService.addBuiltinProvider(this.manifest.origin);
    }
    return val;
  },

  uninstall: function() {
    let prefName = getPrefnameFromOrigin(this.manifest.origin);
    if (Services.prefs.prefHasUserValue(prefName)) {
      if (ActiveProviders.has(this.manifest.origin)) {
        SocialService.removeProvider(this.manifest.origin, function() {
          SocialAddonProvider.removeAddon(this);
        }.bind(this));
      } else {
        SocialAddonProvider.removeAddon(this);
      }
    }
  },

  cancelUninstall: function() {
    let prefName = getPrefnameFromOrigin(this.manifest.origin);
    if (Services.prefs.prefHasUserValue(prefName))
      throw new Error(this.manifest.name + " is not marked to be uninstalled");
    
    let string = Cc["@mozilla.org/supports-string;1"].
                 createInstance(Ci.nsISupportsString);
    string.data = JSON.stringify(this.manifest);
    Services.prefs.setComplexValue(prefName, Ci.nsISupportsString, string);
    this._pending -= AddonManager.PENDING_UNINSTALL;
    AddonManagerPrivate.callAddonListeners("onOperationCancelled", this);
  }
};


AddonManagerPrivate.registerProvider(SocialAddonProvider, [
  new AddonManagerPrivate.AddonType(ADDON_TYPE_SERVICE, URI_EXTENSION_STRINGS,
                                    STRING_TYPE_NAME,
                                    AddonManager.VIEW_TYPE_LIST, 10000)
]);
