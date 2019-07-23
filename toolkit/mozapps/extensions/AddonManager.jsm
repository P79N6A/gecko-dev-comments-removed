






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const PREF_EM_UPDATE_ENABLED = "extensions.update.enabled";

Components.utils.import("resource://gre/modules/Services.jsm");

var EXPORTED_SYMBOLS = [ "AddonManager", "AddonManagerPrivate" ];


const PROVIDERS = [
  "resource://gre/modules/XPIProvider.jsm",
  "resource://gre/modules/LightweightThemeManager.jsm"
];







function LOG(str) {
  dump("*** addons.manager: " + str + "\n");
}







function WARN(str) {
  LOG(str);
}







function ERROR(str) {
  LOG(str);
}








function safeCall(callback) {
  var args = Array.slice(arguments, 1);

  try {
    callback.apply(null, args);
  }
  catch (e) {
    WARN("Exception calling callback: " + e);
  }
}















function callProvider(provider, method, dflt) {
  if (!(method in provider))
    return dflt;

  var args = Array.slice(arguments, 3);

  try {
    return provider[method].apply(provider, args);
  }
  catch (e) {
    ERROR("Exception calling provider." + method + ": " + e);
    return dflt;
  }
}
















function AsyncObjectCaller(objects, method, listener) {
  this.objects = objects.slice(0);
  this.method = method;
  this.listener = listener;

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





var AddonManagerInternal = {
  installListeners: null,
  addonListeners: null,
  providers: [],
  started: false,

  



  startup: function AMI_startup() {
    if (this.started)
      return;

    this.installListeners = [];
    this.addonListeners = [];

    let appChanged = true;

    try {
      appChanged = Services.appinfo.version !=
                   Services.prefs.getCharPref("extensions.lastAppVersion");
    }
    catch (e) { }

    if (appChanged) {
      LOG("Application has been upgraded");
      Services.prefs.setCharPref("extensions.lastAppVersion",
                                 Services.appinfo.version);
    }

    
    PROVIDERS.forEach(function(url) {
      try {
        Components.utils.import(url, {});
      }
      catch (e) {
        ERROR("Exception loading provider \"" + url + "\": " + e);
      }
    });

    let needsRestart = false;
    this.providers.forEach(function(provider) {
      callProvider(provider, "startup");
      if (callProvider(provider, "checkForChanges", false, appChanged))
        needsRestart = true;
    });
    this.started = true;

    
    if (needsRestart) {
      let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].
                       getService(Ci.nsIAppStartup2);
      appStartup.needsRestart = needsRestart;
    }
  },

  





  registerProvider: function AMI_registerProvider(provider) {
    this.providers.push(provider);

    
    if (this.started)
      callProvider(provider, "startup");
  },

  



  shutdown: function AM_shutdown() {
    this.providers.forEach(function(provider) {
      callProvider(provider, "shutdown");
    });

    this.installListeners = null;
    this.addonListeners = null;
    this.started = false;
  },

  



  backgroundUpdateCheck: function AMI_backgroundUpdateCheck() {
    if (!Services.prefs.getBoolPref(PREF_EM_UPDATE_ENABLED))
      return;

    this.getAddonsByTypes(null, function getAddonsCallback(addons) {
      addons.forEach(function BUC_forEachCallback(addon) {
        if (addon.permissions & AddonManager.PERM_CAN_UPGRADE) {
          addon.findUpdates({
            onUpdateAvailable: function BUC_onUpdateAvailable(addon, install) {
              install.install();
            }
          }, AddonManager.UPDATE_WHEN_PERIODIC_UPDATE);
        }
      });
    });
  },

  









  callInstallListeners: function AMI_callInstallListeners(method, extraListeners) {
    let result = true;
    let listeners = this.installListeners;
    if (extraListeners)
      listeners = extraListeners.concat(listeners);
    let args = Array.slice(arguments, 2);

    listeners.forEach(function(listener) {
      try {
        if (method in listener) {
          if (listener[method].apply(listener, args) === false)
            result = false;
        }
      }
      catch (e) {
        WARN("InstallListener threw exception when calling " + method + ": " + e);
      }
    });
    return result;
  },

  






  callAddonListeners: function AMI_callAddonListeners(method) {
    var args = Array.slice(arguments, 1);
    this.addonListeners.forEach(function(listener) {
      try {
        if (method in listener)
          listener[method].apply(listener, args);
      }
      catch (e) {
        WARN("AddonListener threw exception when calling " + method + ": " + e);
      }
    });
  },

  












  notifyAddonChanged: function AMI_notifyAddonChanged(id, type, pendingRestart) {
    this.providers.forEach(function(provider) {
      callProvider(provider, "addonChanged", null, id, type, pendingRestart);
    });
  },

  




















  getInstallForURL: function AMI_getInstallForURL(url, callback, mimetype, hash,
                                                  name, iconURL, version,
                                                  loadgroup) {
    if (!url || !mimetype || !callback)
      throw new TypeError("Invalid arguments");

    for (let i = 0; i < this.providers.length; i++) {
      if (callProvider(this.providers[i], "supportsMimetype", false, mimetype)) {
        callProvider(this.providers[i], "getInstallForURL", null,
                     url, hash, name, iconURL, version, loadgroup,
                     function(install) {
          safeCall(callback, install);
        });
        return;
      }
    }
    safeCall(callback, null);
  },

  










  getInstallForFile: function AMI_getInstallForFile(file, callback, mimetype) {
    if (!file || !callback)
      throw Cr.NS_ERROR_INVALID_ARG;

    new AsyncObjectCaller(this.providers, "getInstallForFile", {
      nextObject: function(caller, provider) {
        callProvider(provider, "getInstallForFile", null, file,
                     function(install) {
          if (install)
            safeCall(callback, install);
          else
            caller.callNext();
        });
      },

      noMoreObjects: function(caller) {
        safeCall(callback, null);
      }
    });
  },

  









  getInstalls: function AMI_getInstalls(types, callback) {
    if (!callback)
      throw Cr.NS_ERROR_INVALID_ARG;

    let installs = [];

    new AsyncObjectCaller(this.providers, "getInstalls", {
      nextObject: function(caller, provider) {
        callProvider(provider, "getInstalls", null, types,
                     function(providerInstalls) {
          installs = installs.concat(providerInstalls);
          caller.callNext();
        });
      },

      noMoreObjects: function(caller) {
        safeCall(callback, installs);
      }
    });
  },

  






  isInstallEnabled: function AMI_isInstallEnabled(mimetype) {
    for (let i = 0; i < this.providers.length; i++) {
      if (callProvider(this.providers[i], "supportsMimetype", false, mimetype) &&
          callProvider(this.providers[i], "isInstallEnabled"))
        return true;
    }
    return false;
  },

  









  isInstallAllowed: function AMI_isInstallAllowed(mimetype, uri) {
    for (let i = 0; i < this.providers.length; i++) {
      if (callProvider(this.providers[i], "supportsMimetype", false, mimetype) &&
          callProvider(this.providers[i], "isInstallAllowed", null, uri))
        return true;
    }
  },

  












  installAddonsFromWebpage: function AMI_installAddonsFromWebpage(mimetype,
                                                                  source,
                                                                  uri,
                                                                  installs) {
    if (!("@mozilla.org/addons/web-install-listener;1" in Cc)) {
      WARN("No web installer available, cancelling all installs");
      installs.forEach(function(install) {
        install.cancel();
      });
      return;
    }

    try {
      let weblistener = Cc["@mozilla.org/addons/web-install-listener;1"].
                        getService(Ci.amIWebInstallListener);

      if (!this.isInstallAllowed(mimetype, uri)) {
        if (weblistener.onWebInstallBlocked(source, uri, installs,
                                            installs.length)) {
          installs.forEach(function(install) {
            install.install();
          });
        }
      }
      else if (weblistener.onWebInstallRequested(source, uri, installs,
                                                 installs.length)) {
        installs.forEach(function(install) {
          install.install();
        });
      }
    }
    catch (e) {
      
      
      
      WARN("Failure calling web installer: " + e);
      installs.forEach(function(install) {
        install.cancel();
      });
    }
  },

  





  addInstallListener: function AMI_addInstallListener(listener) {
    if (!this.installListeners.some(function(i) { return i == listener; }))
      this.installListeners.push(listener);
  },

  





  removeInstallListener: function AMI_removeInstallListener(listener) {
    this.installListeners = this.installListeners.filter(function(i) {
      return i != listener;
    });
  },

  








  getAddon: function AMI_getAddon(id, callback) {
    if (!id || !callback)
      throw Cr.NS_ERROR_INVALID_ARG;

    new AsyncObjectCaller(this.providers, "getAddon", {
      nextObject: function(caller, provider) {
        callProvider(provider, "getAddon", null, id, function(addon) {
          if (addon)
            safeCall(callback, addon);
          else
            caller.callNext();
        });
      },

      noMoreObjects: function(caller) {
        safeCall(callback, null);
      }
    });
  },

  








  getAddons: function AMI_getAddons(ids, callback) {
    if (!ids || !callback)
      throw Cr.NS_ERROR_INVALID_ARG;

    let addons = [];

    new AsyncObjectCaller(ids, null, {
      nextObject: function(caller, id) {
        AddonManagerInternal.getAddon(id, function(addon) {
          addons.push(addon);
          caller.callNext();
        });
      },

      noMoreObjects: function(caller) {
        safeCall(callback, addons);
      }
    });
  },

  








  getAddonsByTypes: function AMI_getAddonsByTypes(types, callback) {
    if (!callback)
      throw Cr.NS_ERROR_INVALID_ARG;

    let addons = [];

    new AsyncObjectCaller(this.providers, "getAddonsByTypes", {
      nextObject: function(caller, provider) {
        callProvider(provider, "getAddonsByTypes", null, types,
                     function(providerAddons) {
          addons = addons.concat(providerAddons);
          caller.callNext();
        });
      },

      noMoreObjects: function(caller) {
        safeCall(callback, addons);
      }
    });
  },

  









  getAddonsWithPendingOperations:
  function AMI_getAddonsWithPendingOperations(types, callback) {
    if (!callback)
      throw Cr.NS_ERROR_INVALID_ARG;

    let addons = [];

    new AsyncObjectCaller(this.providers, "getAddonsWithPendingOperations", {
      nextObject: function(caller, provider) {
        callProvider(provider, "getAddonsWithPendingOperations", null, types,
                     function(providerAddons) {
          addons = addons.concat(providerAddons);
          caller.callNext();
        });
      },

      noMoreObjects: function(caller) {
        safeCall(callback, addons);
      }
    });
  },

  





  addAddonListener: function AMI_addAddonListener(listener) {
    if (!this.addonListeners.some(function(i) { return i == listener; }))
      this.addonListeners.push(listener);
  },

  





  removeAddonListener: function AMI_removeAddonListener(listener) {
    this.addonListeners = this.addonListeners.filter(function(i) {
      return i != listener;
    });
  }
};







var AddonManagerPrivate = {
  startup: function AMP_startup() {
    AddonManagerInternal.startup();
  },

  registerProvider: function AMP_registerProvider(provider) {
    AddonManagerInternal.registerProvider(provider);
  },

  shutdown: function AMP_shutdown() {
    AddonManagerInternal.shutdown();
  },

  backgroundUpdateCheck: function AMP_backgroundUpdateCheck() {
    AddonManagerInternal.backgroundUpdateCheck();
  },

  notifyAddonChanged: function AMP_notifyAddonChanged(id, type, pendingRestart) {
    AddonManagerInternal.notifyAddonChanged(id, type, pendingRestart);
  },

  callInstallListeners: function AMP_callInstallListeners(method) {
    return AddonManagerInternal.callInstallListeners.apply(AddonManagerInternal,
                                                          arguments);
  },

  callAddonListeners: function AMP_callAddonListeners(method) {
    AddonManagerInternal.callAddonListeners.apply(AddonManagerInternal, arguments);
  }
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

  
  
  PERM_CAN_UNINSTALL: 1,
  
  PERM_CAN_ENABLE: 2,
  
  PERM_CAN_DISABLE: 4,
  
  PERM_CAN_UPGRADE: 8,

  getInstallForURL: function AM_getInstallForURL(url, callback, mimetype, hash,
                                                 name, iconURL, version,
                                                 loadgroup) {
    AddonManagerInternal.getInstallForURL(url, callback, mimetype, hash, name,
                                         iconURL, version, loadgroup);
  },

  getInstallForFile: function AM_getInstallForFile(file, callback, mimetype) {
    AddonManagerInternal.getInstallForFile(file, callback, mimetype);
  },

  getAddon: function AM_getAddon(id, callback) {
    AddonManagerInternal.getAddon(id, callback);
  },

  getAddons: function AM_getAddons(ids, callback) {
    AddonManagerInternal.getAddons(ids, callback);
  },

  getAddonsWithPendingOperations:
  function AM_getAddonsWithPendingOperations(types, callback) {
    AddonManagerInternal.getAddonsWithPendingOperations(types, callback);
  },

  getAddonsByTypes: function AM_getAddonsByTypes(types, callback) {
    AddonManagerInternal.getAddonsByTypes(types, callback);
  },

  getInstalls: function AM_getInstalls(types, callback) {
    AddonManagerInternal.getInstalls(types, callback);
  },

  isInstallEnabled: function AM_isInstallEnabled(type) {
    return AddonManagerInternal.isInstallEnabled(type);
  },

  isInstallAllowed: function AM_isInstallAllowed(type, uri) {
    return AddonManagerInternal.isInstallAllowed(type, uri);
  },

  installAddonsFromWebpage: function AM_installAddonsFromWebpage(type, source,
                                                                 uri, installs) {
    AddonManagerInternal.installAddonsFromWebpage(type, source, uri, installs);
  },

  addInstallListener: function AM_addInstallListener(listener) {
    AddonManagerInternal.addInstallListener(listener);
  },

  removeInstallListener: function AM_removeInstallListener(listener) {
    AddonManagerInternal.removeInstallListener(listener);
  },

  addAddonListener: function AM_addAddonListener(listener) {
    AddonManagerInternal.addAddonListener(listener);
  },

  removeAddonListener: function AM_removeAddonListener(listener) {
    AddonManagerInternal.removeAddonListener(listener);
  }
};
