






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const PREF_BLOCKLIST_PINGCOUNTVERSION = "extensions.blocklist.pingCountVersion";
const PREF_EM_UPDATE_ENABLED          = "extensions.update.enabled";
const PREF_EM_LAST_APP_VERSION        = "extensions.lastAppVersion";
const PREF_EM_LAST_PLATFORM_VERSION   = "extensions.lastPlatformVersion";
const PREF_EM_AUTOUPDATE_DEFAULT      = "extensions.update.autoUpdateDefault";

Components.utils.import("resource://gre/modules/Services.jsm");

var EXPORTED_SYMBOLS = [ "AddonManager", "AddonManagerPrivate" ];

const CATEGORY_PROVIDER_MODULE = "addon-provider-module";


const DEFAULT_PROVIDERS = [
  "resource://gre/modules/XPIProvider.jsm",
  "resource://gre/modules/LightweightThemeManager.jsm"
];

["LOG", "WARN", "ERROR"].forEach(function(aName) {
  this.__defineGetter__(aName, function() {
    Components.utils.import("resource://gre/modules/AddonLogging.jsm");

    LogManager.getLogger("addons.manager", this);
    return this[aName];
  });
}, this);








function safeCall(aCallback) {
  var args = Array.slice(arguments, 1);

  try {
    aCallback.apply(null, args);
  }
  catch (e) {
    WARN("Exception calling callback", e);
  }
}















function callProvider(aProvider, aMethod, aDefault) {
  if (!(aMethod in aProvider))
    return aDefault;

  var args = Array.slice(arguments, 3);

  try {
    return aProvider[aMethod].apply(aProvider, args);
  }
  catch (e) {
    ERROR("Exception calling provider " + aMethod, e);
    return aDefault;
  }
}
















function AsyncObjectCaller(aObjects, aMethod, aListener) {
  this.objects = aObjects.slice(0);
  this.method = aMethod;
  this.listener = aListener;

  this.callNext();
}

AsyncObjectCaller.prototype = {
  objects: null,
  method: null,
  listener: null,

  



  callNext: function AOC_callNext() {
    if (this.objects.length == 0) {
      this.listener.noMoreObjects(this);
      return;
    }

    let object = this.objects.shift();
    if (!this.method || this.method in object)
      this.listener.nextObject(this, object);
    else
      this.callNext();
  }
};









function AddonAuthor(aName, aURL) {
  this.name = aName;
  this.url = aURL;
}

AddonAuthor.prototype = {
  name: null,
  url: null,

  
  toString: function() {
    return this.name || "";
  }
}











function AddonScreenshot(aURL, aThumbnailURL, aCaption) {
  this.url = aURL;
  this.thumbnailURL = aThumbnailURL;
  this.caption = aCaption;
}

AddonScreenshot.prototype = {
  url: null,
  thumbnailURL: null,
  caption: null,

  
  toString: function() {
    return this.url || "";
  }
}

var gStarted = false;





var AddonManagerInternal = {
  installListeners: [],
  addonListeners: [],
  providers: [],

  



  startup: function AMI_startup() {
    if (gStarted)
      return;

    let appChanged = undefined;

    let oldAppVersion = null;
    try {
      oldAppVersion = Services.prefs.getCharPref(PREF_EM_LAST_APP_VERSION);
      appChanged = Services.appinfo.version != oldAppVersion;
    }
    catch (e) { }

    let oldPlatformVersion = null;
    try {
      oldPlatformVersion = Services.prefs.getCharPref(PREF_EM_LAST_PLATFORM_VERSION);
    }
    catch (e) { }

    if (appChanged !== false) {
      LOG("Application has been upgraded");
      Services.prefs.setCharPref(PREF_EM_LAST_APP_VERSION,
                                 Services.appinfo.version);
      Services.prefs.setCharPref(PREF_EM_LAST_PLATFORM_VERSION,
                                 Services.appinfo.platformVersion);
      Services.prefs.setIntPref(PREF_BLOCKLIST_PINGCOUNTVERSION,
                                (appChanged === undefined ? 0 : -1));
    }

    
    DEFAULT_PROVIDERS.forEach(function(url) {
      try {
        Components.utils.import(url, {});
      }
      catch (e) {
        ERROR("Exception loading default provider \"" + url + "\"", e);
      }
    });

    
    let catman = Cc["@mozilla.org/categorymanager;1"].
                 getService(Ci.nsICategoryManager);
    let entries = catman.enumerateCategory(CATEGORY_PROVIDER_MODULE);
    while (entries.hasMoreElements()) {
      let entry = entries.getNext().QueryInterface(Ci.nsISupportsCString).data;
      let url = catman.getCategoryEntry(CATEGORY_PROVIDER_MODULE, entry);

      try {
        Components.utils.import(url, {});
      }
      catch (e) {
        ERROR("Exception loading provider " + entry + " from category \"" +
              url + "\"", e);
      }
    }

    this.providers.forEach(function(provider) {
      callProvider(provider, "startup", null, appChanged, oldAppVersion,
                   oldPlatformVersion);
    });
    gStarted = true;
  },

  





  registerProvider: function AMI_registerProvider(aProvider) {
    this.providers.push(aProvider);

    
    if (gStarted)
      callProvider(aProvider, "startup");
  },

  





  unregisterProvider: function AMI_unregisterProvider(aProvider) {
    let pos = 0;
    while (pos < this.providers.length) {
      if (this.providers[pos] == aProvider)
        this.providers.splice(pos, 1);
      else
        pos++;
    }

    
    if (gStarted)
      callProvider(aProvider, "shutdown");
  },

  



  shutdown: function AM_shutdown() {
    this.providers.forEach(function(provider) {
      callProvider(provider, "shutdown");
    });

    this.installListeners.splice(0);
    this.addonListeners.splice(0);
    gStarted = false;
  },

  



  backgroundUpdateCheck: function AMI_backgroundUpdateCheck() {
    if (!Services.prefs.getBoolPref(PREF_EM_UPDATE_ENABLED))
      return;

    Services.obs.notifyObservers(null, "addons-background-update-start", null);
    let pendingUpdates = 1;

    function notifyComplete() {
      if (--pendingUpdates == 0)
        Services.obs.notifyObservers(null, "addons-background-update-complete", null);
    }

    let scope = {};
    Components.utils.import("resource://gre/modules/AddonRepository.jsm", scope);
    Components.utils.import("resource://gre/modules/LightweightThemeManager.jsm", scope);
    scope.LightweightThemeManager.updateCurrentTheme();

    this.getAllAddons(function getAddonsCallback(aAddons) {
      pendingUpdates++;
      var ids = [a.id for each (a in aAddons)];
      scope.AddonRepository.repopulateCache(ids, notifyComplete);

      pendingUpdates += aAddons.length;
      var autoUpdateDefault = AddonManager.autoUpdateDefault;

      function shouldAutoUpdate(aAddon) {
        if (!("applyBackgroundUpdates" in aAddon))
          return false;
        if (aAddon.applyBackgroundUpdates == AddonManager.AUTOUPDATE_ENABLE)
          return true;
        if (aAddon.applyBackgroundUpdates == AddonManager.AUTOUPDATE_DISABLE)
          return false;
        return autoUpdateDefault;
      }

      aAddons.forEach(function BUC_forEachCallback(aAddon) {
        
        
        aAddon.findUpdates({
          onUpdateAvailable: function BUC_onUpdateAvailable(aAddon, aInstall) {
            
            
            if (aAddon.permissions & AddonManager.PERM_CAN_UPGRADE &&
                shouldAutoUpdate(aAddon)) {
              aInstall.install();
            }
          },

          onUpdateFinished: notifyComplete
        }, AddonManager.UPDATE_WHEN_PERIODIC_UPDATE);
      });

      notifyComplete();
    });
  },

  









  callInstallListeners: function AMI_callInstallListeners(aMethod, aExtraListeners) {
    let result = true;
    let listeners = this.installListeners;
    if (aExtraListeners)
      listeners = aExtraListeners.concat(listeners);
    let args = Array.slice(arguments, 2);

    listeners.forEach(function(listener) {
      try {
        if (aMethod in listener) {
          if (listener[aMethod].apply(listener, args) === false)
            result = false;
        }
      }
      catch (e) {
        WARN("InstallListener threw exception when calling " + aMethod, e);
      }
    });
    return result;
  },

  






  callAddonListeners: function AMI_callAddonListeners(aMethod) {
    var args = Array.slice(arguments, 1);
    this.addonListeners.forEach(function(listener) {
      try {
        if (aMethod in listener)
          listener[aMethod].apply(listener, args);
      }
      catch (e) {
        WARN("AddonListener threw exception when calling " + aMethod, e);
      }
    });
  },

  












  notifyAddonChanged: function AMI_notifyAddonChanged(aId, aType, aPendingRestart) {
    this.providers.forEach(function(provider) {
      callProvider(provider, "addonChanged", null, aId, aType, aPendingRestart);
    });
  },

  




  updateAddonAppDisabledStates: function AMI_updateAddonAppDisabledStates() {
    this.providers.forEach(function(provider) {
      callProvider(provider, "updateAddonAppDisabledStates");
    });
  },

  




















  getInstallForURL: function AMI_getInstallForURL(aUrl, aCallback, aMimetype,
                                                  aHash, aName, aIconURL,
                                                  aVersion, aLoadGroup) {
    if (!aUrl || !aMimetype || !aCallback)
      throw new TypeError("Invalid arguments");

    for (let i = 0; i < this.providers.length; i++) {
      if (callProvider(this.providers[i], "supportsMimetype", false, aMimetype)) {
        callProvider(this.providers[i], "getInstallForURL", null,
                     aUrl, aHash, aName, aIconURL, aVersion, aLoadGroup,
                     function(aInstall) {
          safeCall(aCallback, aInstall);
        });
        return;
      }
    }
    safeCall(aCallback, null);
  },

  










  getInstallForFile: function AMI_getInstallForFile(aFile, aCallback, aMimetype) {
    if (!aFile || !aCallback)
      throw Cr.NS_ERROR_INVALID_ARG;

    new AsyncObjectCaller(this.providers, "getInstallForFile", {
      nextObject: function(aCaller, aProvider) {
        callProvider(aProvider, "getInstallForFile", null, aFile,
                     function(aInstall) {
          if (aInstall)
            safeCall(aCallback, aInstall);
          else
            aCaller.callNext();
        });
      },

      noMoreObjects: function(aCaller) {
        safeCall(aCallback, null);
      }
    });
  },

  









  getInstallsByTypes: function AMI_getInstallsByTypes(aTypes, aCallback) {
    if (!aCallback)
      throw Cr.NS_ERROR_INVALID_ARG;

    let installs = [];

    new AsyncObjectCaller(this.providers, "getInstallsByTypes", {
      nextObject: function(aCaller, aProvider) {
        callProvider(aProvider, "getInstallsByTypes", null, aTypes,
                     function(aProviderInstalls) {
          installs = installs.concat(aProviderInstalls);
          aCaller.callNext();
        });
      },

      noMoreObjects: function(aCaller) {
        safeCall(aCallback, installs);
      }
    });
  },

  





  getAllInstalls: function AMI_getAllInstalls(aCallback) {
    this.getInstallsByTypes(null, aCallback);
  },

  






  isInstallEnabled: function AMI_isInstallEnabled(aMimetype) {
    for (let i = 0; i < this.providers.length; i++) {
      if (callProvider(this.providers[i], "supportsMimetype", false, aMimetype) &&
          callProvider(this.providers[i], "isInstallEnabled"))
        return true;
    }
    return false;
  },

  









  isInstallAllowed: function AMI_isInstallAllowed(aMimetype, aURI) {
    for (let i = 0; i < this.providers.length; i++) {
      if (callProvider(this.providers[i], "supportsMimetype", false, aMimetype) &&
          callProvider(this.providers[i], "isInstallAllowed", null, aURI))
        return true;
    }
    return false;
  },

  












  installAddonsFromWebpage: function AMI_installAddonsFromWebpage(aMimetype,
                                                                  aSource,
                                                                  aURI,
                                                                  aInstalls) {
    if (!("@mozilla.org/addons/web-install-listener;1" in Cc)) {
      WARN("No web installer available, cancelling all installs");
      aInstalls.forEach(function(aInstall) {
        aInstall.cancel();
      });
      return;
    }

    try {
      let weblistener = Cc["@mozilla.org/addons/web-install-listener;1"].
                        getService(Ci.amIWebInstallListener);

      if (!this.isInstallEnabled(aMimetype, aURI)) {
        weblistener.onWebInstallDisabled(aSource, aURI, aInstalls,
                                         aInstalls.length);
      }
      else if (!this.isInstallAllowed(aMimetype, aURI)) {
        if (weblistener.onWebInstallBlocked(aSource, aURI, aInstalls,
                                            aInstalls.length)) {
          aInstalls.forEach(function(aInstall) {
            aInstall.install();
          });
        }
      }
      else if (weblistener.onWebInstallRequested(aSource, aURI, aInstalls,
                                                   aInstalls.length)) {
        aInstalls.forEach(function(aInstall) {
          aInstall.install();
        });
      }
    }
    catch (e) {
      
      
      
      WARN("Failure calling web installer", e);
      aInstalls.forEach(function(aInstall) {
        aInstall.cancel();
      });
    }
  },

  





  addInstallListener: function AMI_addInstallListener(aListener) {
    if (!this.installListeners.some(function(i) { return i == aListener; }))
      this.installListeners.push(aListener);
  },

  





  removeInstallListener: function AMI_removeInstallListener(aListener) {
    let pos = 0;
    while (pos < this.installListeners.length) {
      if (this.installListeners[pos] == aListener)
        this.installListeners.splice(pos, 1);
      else
        pos++;
    }
  },

  








  getAddonByID: function AMI_getAddonByID(aId, aCallback) {
    if (!aId || !aCallback)
      throw Cr.NS_ERROR_INVALID_ARG;

    new AsyncObjectCaller(this.providers, "getAddonByID", {
      nextObject: function(aCaller, aProvider) {
        callProvider(aProvider, "getAddonByID", null, aId, function(aAddon) {
          if (aAddon)
            safeCall(aCallback, aAddon);
          else
            aCaller.callNext();
        });
      },

      noMoreObjects: function(aCaller) {
        safeCall(aCallback, null);
      }
    });
  },

  








  getAddonsByIDs: function AMI_getAddonsByIDs(aIds, aCallback) {
    if (!aIds || !aCallback)
      throw Cr.NS_ERROR_INVALID_ARG;

    let addons = [];

    new AsyncObjectCaller(aIds, null, {
      nextObject: function(aCaller, aId) {
        AddonManagerInternal.getAddonByID(aId, function(aAddon) {
          addons.push(aAddon);
          aCaller.callNext();
        });
      },

      noMoreObjects: function(aCaller) {
        safeCall(aCallback, addons);
      }
    });
  },

  








  getAddonsByTypes: function AMI_getAddonsByTypes(aTypes, aCallback) {
    if (!aCallback)
      throw Cr.NS_ERROR_INVALID_ARG;

    let addons = [];

    new AsyncObjectCaller(this.providers, "getAddonsByTypes", {
      nextObject: function(aCaller, aProvider) {
        callProvider(aProvider, "getAddonsByTypes", null, aTypes,
                     function(aProviderAddons) {
          addons = addons.concat(aProviderAddons);
          aCaller.callNext();
        });
      },

      noMoreObjects: function(aCaller) {
        safeCall(aCallback, addons);
      }
    });
  },

  





  getAllAddons: function AMI_getAllAddons(aCallback) {
    this.getAddonsByTypes(null, aCallback);
  },

  









  getAddonsWithOperationsByTypes:
  function AMI_getAddonsWithOperationsByTypes(aTypes, aCallback) {
    if (!aCallback)
      throw Cr.NS_ERROR_INVALID_ARG;

    let addons = [];

    new AsyncObjectCaller(this.providers, "getAddonsWithOperationsByTypes", {
      nextObject: function(aCaller, aProvider) {
        callProvider(aProvider, "getAddonsWithOperationsByTypes", null, aTypes,
                     function(aProviderAddons) {
          addons = addons.concat(aProviderAddons);
          aCaller.callNext();
        });
      },

      noMoreObjects: function(caller) {
        safeCall(aCallback, addons);
      }
    });
  },

  





  addAddonListener: function AMI_addAddonListener(aListener) {
    if (!this.addonListeners.some(function(i) { return i == aListener; }))
      this.addonListeners.push(aListener);
  },

  





  removeAddonListener: function AMI_removeAddonListener(aListener) {
    let pos = 0;
    while (pos < this.addonListeners.length) {
      if (this.addonListeners[pos] == aListener)
        this.addonListeners.splice(pos, 1);
      else
        pos++;
    }
  },

  get autoUpdateDefault() {
    try {
      return Services.prefs.getBoolPref(PREF_EM_AUTOUPDATE_DEFAULT);
    } catch(e) { }
    return true;
  }
};







var AddonManagerPrivate = {
  startup: function AMP_startup() {
    AddonManagerInternal.startup();
  },

  registerProvider: function AMP_registerProvider(aProvider) {
    AddonManagerInternal.registerProvider(aProvider);
  },

  unregisterProvider: function AMP_unregisterProvider(aProvider) {
    AddonManagerInternal.unregisterProvider(aProvider);
  },

  shutdown: function AMP_shutdown() {
    AddonManagerInternal.shutdown();
  },

  backgroundUpdateCheck: function AMP_backgroundUpdateCheck() {
    AddonManagerInternal.backgroundUpdateCheck();
  },

  notifyAddonChanged: function AMP_notifyAddonChanged(aId, aType, aPendingRestart) {
    AddonManagerInternal.notifyAddonChanged(aId, aType, aPendingRestart);
  },

  updateAddonAppDisabledStates: function AMP_updateAddonAppDisabledStates() {
    AddonManagerInternal.updateAddonAppDisabledStates();
  },

  callInstallListeners: function AMP_callInstallListeners(aMethod) {
    return AddonManagerInternal.callInstallListeners.apply(AddonManagerInternal,
                                                           arguments);
  },

  callAddonListeners: function AMP_callAddonListeners(aMethod) {
    AddonManagerInternal.callAddonListeners.apply(AddonManagerInternal, arguments);
  },

  AddonAuthor: AddonAuthor,

  AddonScreenshot: AddonScreenshot
};





var AddonManager = {
  
  
  STATE_AVAILABLE: 0,
  
  STATE_DOWNLOADING: 1,
  
  STATE_CHECKING: 2,
  
  STATE_DOWNLOADED: 3,
  
  STATE_DOWNLOAD_FAILED: 4,
  
  STATE_INSTALLING: 5,
  
  STATE_INSTALLED: 6,
  
  STATE_INSTALL_FAILED: 7,
  
  STATE_CANCELLED: 8,

  
  
  
  ERROR_NETWORK_FAILURE: -1,
  
  ERROR_INCORRECT_HASH: -2,
  
  ERROR_CORRUPT_FILE: -3,
  
  ERROR_FILE_ACCESS: -4,

  
  
  UPDATE_STATUS_NO_ERROR: 0,
  
  UPDATE_STATUS_TIMEOUT: -1,
  
  UPDATE_STATUS_DOWNLOAD_ERROR: -2,
  
  UPDATE_STATUS_PARSE_ERROR: -3,
  
  UPDATE_STATUS_UNKNOWN_FORMAT: -4,
  
  UPDATE_STATUS_SECURITY_ERROR: -5,

  
  
  UPDATE_WHEN_USER_REQUESTED: 1,
  
  
  UPDATE_WHEN_NEW_APP_DETECTED: 2,
  
  UPDATE_WHEN_NEW_APP_INSTALLED: 3,
  
  UPDATE_WHEN_PERIODIC_UPDATE: 16,
  
  UPDATE_WHEN_ADDON_INSTALLED: 17,

  
  
  PENDING_NONE: 0,
  
  PENDING_ENABLE: 1,
  
  PENDING_DISABLE: 2,
  
  PENDING_UNINSTALL: 4,
  
  PENDING_INSTALL: 8,
  PENDING_UPGRADE: 16,

  
  
  OP_NEEDS_RESTART_NONE: 0,
  
  OP_NEEDS_RESTART_ENABLE: 1,
  
  OP_NEEDS_RESTART_DISABLE: 2,
  
  OP_NEEDS_RESTART_UNINSTALL: 4,
  
  OP_NEEDS_RESTART_INSTALL: 8,

  
  
  PERM_CAN_UNINSTALL: 1,
  
  PERM_CAN_ENABLE: 2,
  
  PERM_CAN_DISABLE: 4,
  
  PERM_CAN_UPGRADE: 8,

  
  
  SCOPE_PROFILE: 1,
  
  SCOPE_USER: 2,
  
  SCOPE_APPLICATION: 4,
  
  SCOPE_SYSTEM: 8,
  
  SCOPE_ALL: 15,

  
  
  AUTOUPDATE_DISABLE: 0,
  
  
  AUTOUPDATE_DEFAULT: 1,
  
  AUTOUPDATE_ENABLE: 2,

  getInstallForURL: function AM_getInstallForURL(aUrl, aCallback, aMimetype,
                                                 aHash, aName, aIconURL,
                                                 aVersion, aLoadGroup) {
    AddonManagerInternal.getInstallForURL(aUrl, aCallback, aMimetype, aHash,
                                          aName, aIconURL, aVersion, aLoadGroup);
  },

  getInstallForFile: function AM_getInstallForFile(aFile, aCallback, aMimetype) {
    AddonManagerInternal.getInstallForFile(aFile, aCallback, aMimetype);
  },

  getAddonByID: function AM_getAddonByID(aId, aCallback) {
    AddonManagerInternal.getAddonByID(aId, aCallback);
  },

  getAddonsByIDs: function AM_getAddonsByIDs(aIds, aCallback) {
    AddonManagerInternal.getAddonsByIDs(aIds, aCallback);
  },

  getAddonsWithOperationsByTypes:
  function AM_getAddonsWithOperationsByTypes(aTypes, aCallback) {
    AddonManagerInternal.getAddonsWithOperationsByTypes(aTypes, aCallback);
  },

  getAddonsByTypes: function AM_getAddonsByTypes(aTypes, aCallback) {
    AddonManagerInternal.getAddonsByTypes(aTypes, aCallback);
  },

  getAllAddons: function AM_getAllAddons(aCallback) {
    AddonManagerInternal.getAllAddons(aCallback);
  },

  getInstallsByTypes: function AM_getInstallsByTypes(aTypes, aCallback) {
    AddonManagerInternal.getInstallsByTypes(aTypes, aCallback);
  },

  getAllInstalls: function AM_getAllInstalls(aCallback) {
    AddonManagerInternal.getAllInstalls(aCallback);
  },

  isInstallEnabled: function AM_isInstallEnabled(aType) {
    return AddonManagerInternal.isInstallEnabled(aType);
  },

  isInstallAllowed: function AM_isInstallAllowed(aType, aUri) {
    return AddonManagerInternal.isInstallAllowed(aType, aUri);
  },

  installAddonsFromWebpage: function AM_installAddonsFromWebpage(aType, aSource,
                                                                 aUri, aInstalls) {
    AddonManagerInternal.installAddonsFromWebpage(aType, aSource, aUri, aInstalls);
  },

  addInstallListener: function AM_addInstallListener(aListener) {
    AddonManagerInternal.addInstallListener(aListener);
  },

  removeInstallListener: function AM_removeInstallListener(aListener) {
    AddonManagerInternal.removeInstallListener(aListener);
  },

  addAddonListener: function AM_addAddonListener(aListener) {
    AddonManagerInternal.addAddonListener(aListener);
  },

  removeAddonListener: function AM_removeAddonListener(aListener) {
    AddonManagerInternal.removeAddonListener(aListener);
  },

  get autoUpdateDefault() {
    return AddonManagerInternal.autoUpdateDefault;
  }
};

Object.freeze(AddonManagerInternal);
Object.freeze(AddonManagerPrivate);
Object.freeze(AddonManager);
