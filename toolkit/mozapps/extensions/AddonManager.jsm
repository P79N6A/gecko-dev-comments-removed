







































"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const PREF_BLOCKLIST_PINGCOUNTVERSION = "extensions.blocklist.pingCountVersion";
const PREF_EM_UPDATE_ENABLED          = "extensions.update.enabled";
const PREF_EM_LAST_APP_VERSION        = "extensions.lastAppVersion";
const PREF_EM_LAST_PLATFORM_VERSION   = "extensions.lastPlatformVersion";
const PREF_EM_AUTOUPDATE_DEFAULT      = "extensions.update.autoUpdateDefault";
const PREF_EM_STRICT_COMPATIBILITY    = "extensions.strictCompatibility";
const PREF_EM_CHECK_UPDATE_SECURITY   = "extensions.checkUpdateSecurity";
const PREF_EM_UPDATE_URL              = "extensions.update.url";
const PREF_APP_UPDATE_ENABLED         = "app.update.enabled";
const PREF_APP_UPDATE_AUTO            = "app.update.auto";
const PREF_EM_HOTFIX_ID               = "extensions.hotfix.id";
const PREF_EM_HOTFIX_LASTVERSION      = "extensions.hotfix.lastVersion";
const PREF_EM_HOTFIX_URL              = "extensions.hotfix.url";
const PREF_EM_CERT_CHECKATTRIBUTES    = "extensions.hotfix.cert.checkAttributes";
const PREF_EM_HOTFIX_CERTS            = "extensions.hotfix.certs.";
const PREF_MATCH_OS_LOCALE            = "intl.locale.matchOS";
const PREF_SELECTED_LOCALE            = "general.useragent.locale";

const UPDATE_REQUEST_VERSION          = 2;
const CATEGORY_UPDATE_PARAMS          = "extension-update-params";

const BRANCH_REGEXP                   = /^([^\.]+\.[0-9]+[a-z]*).*/gi;
const PREF_EM_CHECK_COMPATIBILITY_BASE = "extensions.checkCompatibility";
#ifdef MOZ_COMPATIBILITY_NIGHTLY
var PREF_EM_CHECK_COMPATIBILITY = PREF_EM_CHECK_COMPATIBILITY_BASE + ".nightly";
#else
var PREF_EM_CHECK_COMPATIBILITY;
#endif

const TOOLKIT_ID                      = "toolkit@mozilla.org";

const VALID_TYPES_REGEXP = /^[\w\-]+$/;

Components.utils.import("resource://gre/modules/Services.jsm");
var CertUtils = {};
Components.utils.import("resource://gre/modules/CertUtils.jsm", CertUtils);

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





function getLocale() {
  try {
    if (Services.prefs.getBoolPref(PREF_MATCH_OS_LOCALE))
      return Services.locale.getLocaleComponentForUserAgent();
  }
  catch (e) { }

  try {
    let locale = Services.prefs.getComplexValue(PREF_SELECTED_LOCALE,
                                                Ci.nsIPrefLocalizedString);
    if (locale)
      return locale;
  }
  catch (e) { }

  try {
    return Services.prefs.getCharPref(PREF_SELECTED_LOCALE);
  }
  catch (e) { }

  return "en-US";
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



















function AddonScreenshot(aURL, aWidth, aHeight, aThumbnailURL,
                         aThumbnailWidth, aThumbnailHeight, aCaption) {
  this.url = aURL;
  if (aWidth) this.width = aWidth;
  if (aHeight) this.height = aHeight;
  if (aThumbnailURL) this.thumbnailURL = aThumbnailURL;
  if (aThumbnailWidth) this.thumbnailWidth = aThumbnailWidth;
  if (aThumbnailHeight) this.thumbnailHeight = aThumbnailHeight;
  if (aCaption) this.caption = aCaption;
}

AddonScreenshot.prototype = {
  url: null,
  width: null,
  height: null,
  thumbnailURL: null,
  thumbnailWidth: null,
  thumbnailHeight: null,
  caption: null,

  
  toString: function() {
    return this.url || "";
  }
}


















function AddonCompatibilityOverride(aType, aMinVersion, aMaxVersion, aAppID,
                                    aAppMinVersion, aAppMaxVersion) {
  this.type = aType;
  this.minVersion = aMinVersion;
  this.maxVersion = aMaxVersion;
  this.appID = aAppID;
  this.appMinVersion = aAppMinVersion;
  this.appMaxVersion = aAppMaxVersion;
}

AddonCompatibilityOverride.prototype = {
  



  type: null,

  


  minVersion: null,

  


  maxVersion: null,

  


  appID: null,

  


  appMinVersion: null,

  


  appMaxVersion: null
};
























function AddonType(aId, aLocaleURI, aLocaleKey, aViewType, aUIPriority, aFlags) {
  if (!aId)
    throw new Error("An AddonType must have an ID");
  if (aViewType && aUIPriority === undefined)
    throw new Error("An AddonType with a defined view must have a set UI priority");
  if (!aLocaleKey)
    throw new Error("An AddonType must have a displayable name");

  this.id = aId;
  this.uiPriority = aUIPriority;
  this.viewType = aViewType;
  this.flags = aFlags;

  if (aLocaleURI) {
    this.__defineGetter__("name", function() {
      delete this.name;
      let bundle = Services.strings.createBundle(aLocaleURI);
      this.name = bundle.GetStringFromName(aLocaleKey.replace("%ID%", aId));
      return this.name;
    });
  }
  else {
    this.name = aLocaleKey;
  }
}

var gStarted = false;
var gCheckCompatibility = true;
var gStrictCompatibility = true;
var gCheckUpdateSecurityDefault = true;
var gCheckUpdateSecurity = gCheckUpdateSecurityDefault;
var gUpdateEnabled = true;
var gAutoUpdateDefault = true;





var AddonManagerInternal = {
  managerListeners: [],
  installListeners: [],
  addonListeners: [],
  typeListeners: [],
  providers: [],
  types: {},
  startupChanges: {},

  
  typesProxy: Proxy.create({
    getOwnPropertyDescriptor: function(aName) {
      if (!(aName in AddonManagerInternal.types))
        return undefined;

      return {
        value: AddonManagerInternal.types[aName].type,
        writable: false,
        configurable: false,
        enumerable: true
      }
    },

    getPropertyDescriptor: function(aName) {
      return this.getOwnPropertyDescriptor(aName);
    },

    getOwnPropertyNames: function() {
      return Object.keys(AddonManagerInternal.types);
    },

    getPropertyNames: function() {
      return this.getOwnPropertyNames();
    },

    delete: function(aName) {
      
      return false;
    },

    defineProperty: function(aName, aProperty) {
      
    },

    fix: function() {
      return undefined;
    },

    
    
    enumerate: function() {
      
      return this.getPropertyNames();
    }
  }),

  



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

#ifndef MOZ_COMPATIBILITY_NIGHTLY
    PREF_EM_CHECK_COMPATIBILITY = PREF_EM_CHECK_COMPATIBILITY_BASE + "." +
                                  Services.appinfo.version.replace(BRANCH_REGEXP, "$1");
#endif

    try {
      gCheckCompatibility = Services.prefs.getBoolPref(PREF_EM_CHECK_COMPATIBILITY);
    } catch (e) {}
    Services.prefs.addObserver(PREF_EM_CHECK_COMPATIBILITY, this, false);

    try {
      gStrictCompatibility = Services.prefs.getBoolPref(PREF_EM_STRICT_COMPATIBILITY);
    } catch (e) {}
    Services.prefs.addObserver(PREF_EM_STRICT_COMPATIBILITY, this, false);

    try {
      let defaultBranch = Services.prefs.getDefaultBranch("");
      gCheckUpdateSecurityDefault = defaultBranch.getBoolPref(PREF_EM_CHECK_UPDATE_SECURITY);
    } catch(e) {}

    try {
      gCheckUpdateSecurity = Services.prefs.getBoolPref(PREF_EM_CHECK_UPDATE_SECURITY);
    } catch (e) {}
    Services.prefs.addObserver(PREF_EM_CHECK_UPDATE_SECURITY, this, false);

    try {
      gUpdateEnabled = Services.prefs.getBoolPref(PREF_EM_UPDATE_ENABLED);
    } catch (e) {}
    Services.prefs.addObserver(PREF_EM_UPDATE_ENABLED, this, false);

    try {
      gAutoUpdateDefault = Services.prefs.getBoolPref(PREF_EM_AUTOUPDATE_DEFAULT);
    } catch (e) {}
    Services.prefs.addObserver(PREF_EM_AUTOUPDATE_DEFAULT, this, false);

    
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

    
    if (appChanged === undefined) {
      for (let type in this.startupChanges)
        delete this.startupChanges[type];
    }

    gStarted = true;
  },

  







  registerProvider: function AMI_registerProvider(aProvider, aTypes) {
    this.providers.push(aProvider);

    if (aTypes) {
      aTypes.forEach(function(aType) {
        if (!(aType.id in this.types)) {
          if (!VALID_TYPES_REGEXP.test(aType.id)) {
            WARN("Ignoring invalid type " + aType.id);
            return;
          }

          this.types[aType.id] = {
            type: aType,
            providers: [aProvider]
          };

          this.typeListeners.forEach(function(aListener) {
            safeCall(function() {
              aListener.onTypeAdded(aType);
            });
          });
        }
        else {
          this.types[aType.id].providers.push(aProvider);
        }
      }, this);
    }

    
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

    for (let type in this.types) {
      this.types[type].providers = this.types[type].providers.filter(function(p) p != aProvider);
      if (this.types[type].providers.length == 0) {
        let oldType = this.types[type].type;
        delete this.types[type];

        this.typeListeners.forEach(function(aListener) {
          safeCall(function() {
            aListener.onTypeRemoved(oldType);
          });
        });
      }
    }

    
    if (gStarted)
      callProvider(aProvider, "shutdown");
  },

  



  shutdown: function AMI_shutdown() {
    Services.prefs.removeObserver(PREF_EM_CHECK_COMPATIBILITY, this);
    Services.prefs.removeObserver(PREF_EM_STRICT_COMPATIBILITY, this);
    Services.prefs.removeObserver(PREF_EM_CHECK_UPDATE_SECURITY, this);
    Services.prefs.removeObserver(PREF_EM_UPDATE_ENABLED, this);
    Services.prefs.removeObserver(PREF_EM_AUTOUPDATE_DEFAULT, this);

    this.providers.forEach(function(provider) {
      callProvider(provider, "shutdown");
    });

    this.managerListeners.splice(0, this.managerListeners.length);
    this.installListeners.splice(0, this.installListeners.length);
    this.addonListeners.splice(0, this.addonListeners.length);
    this.typeListeners.splice(0, this.typeListeners.length);
    for (let type in this.startupChanges)
      delete this.startupChanges[type];
    gStarted = false;
  },

  




  observe: function AMI_observe(aSubject, aTopic, aData) {
    switch (aData) {
      case PREF_EM_CHECK_COMPATIBILITY: {
        let oldValue = gCheckCompatibility;
        try {
          gCheckCompatibility = Services.prefs.getBoolPref(PREF_EM_CHECK_COMPATIBILITY);
        } catch(e) {
          gCheckCompatibility = true;
        }

        this.callManagerListeners("onCompatibilityModeChanged");

        if (gCheckCompatibility != oldValue)
          this.updateAddonAppDisabledStates();

        break;
      }
      case PREF_EM_STRICT_COMPATIBILITY: {
        let oldValue = gStrictCompatibility;
        try {
          gStrictCompatibility = Services.prefs.getBoolPref(PREF_EM_STRICT_COMPATIBILITY);
        } catch(e) {
          gStrictCompatibility = true;
        }

        this.callManagerListeners("onCompatibilityModeChanged");

        
        
        
        if (gStrictCompatibility != oldValue)
          this.updateAddonAppDisabledStates();

        break;
      }
      case PREF_EM_CHECK_UPDATE_SECURITY: {
        let oldValue = gCheckUpdateSecurity;
        try {
          gCheckUpdateSecurity = Services.prefs.getBoolPref(PREF_EM_CHECK_UPDATE_SECURITY);
        } catch(e) {
          gCheckUpdateSecurity = true;
        }

        this.callManagerListeners("onCheckUpdateSecurityChanged");

        if (gCheckUpdateSecurity != oldValue)
          this.updateAddonAppDisabledStates();

        break;
      }
      case PREF_EM_UPDATE_ENABLED: {
        let oldValue = gUpdateEnabled;
        try {
          gUpdateEnabled = Services.prefs.getBoolPref(PREF_EM_UPDATE_ENABLED);
        } catch(e) {
          gUpdateEnabled = true;
        }

        this.callManagerListeners("onUpdateModeChanged");
        break;
      }
      case PREF_EM_AUTOUPDATE_DEFAULT: {
        let oldValue = gAutoUpdateDefault;
        try {
          gAutoUpdateDefault = Services.prefs.getBoolPref(PREF_EM_AUTOUPDATE_DEFAULT);
        } catch(e) {
          gAutoUpdateDefault = true;
        }

        this.callManagerListeners("onUpdateModeChanged");
        break;
      }
    }
  },

  











  escapeAddonURI: function AMI_escapeAddonURI(aAddon, aUri, aAppVersion)
  {
    var addonStatus = aAddon.userDisabled || aAddon.softDisabled ? "userDisabled"
                                                                 : "userEnabled";

    if (!aAddon.isCompatible)
      addonStatus += ",incompatible";
    if (aAddon.blocklistState == Ci.nsIBlocklistService.STATE_BLOCKED)
      addonStatus += ",blocklisted";
    if (aAddon.blocklistState == Ci.nsIBlocklistService.STATE_SOFTBLOCKED)
      addonStatus += ",softblocked";

    try {
      var xpcomABI = Services.appinfo.XPCOMABI;
    } catch (ex) {
      xpcomABI = UNKNOWN_XPCOM_ABI;
    }

    let uri = aUri.replace(/%ITEM_ID%/g, aAddon.id);
    uri = uri.replace(/%ITEM_VERSION%/g, aAddon.version);
    uri = uri.replace(/%ITEM_STATUS%/g, addonStatus);
    uri = uri.replace(/%APP_ID%/g, Services.appinfo.ID);
    uri = uri.replace(/%APP_VERSION%/g, aAppVersion ? aAppVersion :
                                                      Services.appinfo.version);
    uri = uri.replace(/%REQ_VERSION%/g, UPDATE_REQUEST_VERSION);
    uri = uri.replace(/%APP_OS%/g, Services.appinfo.OS);
    uri = uri.replace(/%APP_ABI%/g, xpcomABI);
    uri = uri.replace(/%APP_LOCALE%/g, getLocale());
    uri = uri.replace(/%CURRENT_APP_VERSION%/g, Services.appinfo.version);

    
    
    var catMan = null;
    uri = uri.replace(/%(\w{3,})%/g, function(aMatch, aParam) {
      if (!catMan) {
        catMan = Cc["@mozilla.org/categorymanager;1"].
                 getService(Ci.nsICategoryManager);
      }

      try {
        var contractID = catMan.getCategoryEntry(CATEGORY_UPDATE_PARAMS, aParam);
        var paramHandler = Cc[contractID].getService(Ci.nsIPropertyBag2);
        return paramHandler.getPropertyAsAString(aParam);
      }
      catch(e) {
        return aMatch;
      }
    });

    
    return uri.replace(/\+/g, "%2B");
  },

  



  backgroundUpdateCheck: function AMI_backgroundUpdateCheck() {
    let hotfixID = null;
    if (Services.prefs.getPrefType(PREF_EM_HOTFIX_ID) == Ci.nsIPrefBranch.PREF_STRING)
      hotfixID = Services.prefs.getCharPref(PREF_EM_HOTFIX_ID);

    let checkHotfix = hotfixID &&
                      Services.prefs.getBoolPref(PREF_APP_UPDATE_ENABLED) &&
                      Services.prefs.getBoolPref(PREF_APP_UPDATE_AUTO);

    if (!this.updateEnabled && !checkHotfix)
      return;

    Services.obs.notifyObservers(null, "addons-background-update-start", null);

    
    
    
    
    let pendingUpdates = 1;

    function notifyComplete() {
      if (--pendingUpdates == 0) {
        Services.obs.notifyObservers(null,
                                     "addons-background-update-complete",
                                     null);
      }
    }

    if (this.updateEnabled) {
      let scope = {};
      Components.utils.import("resource://gre/modules/AddonRepository.jsm", scope);
      Components.utils.import("resource://gre/modules/LightweightThemeManager.jsm", scope);
      scope.LightweightThemeManager.updateCurrentTheme();

      pendingUpdates++;
      this.getAllAddons(function getAddonsCallback(aAddons) {
        
        var ids = [a.id for each (a in aAddons) if (a.id != hotfixID)];

        
        
        scope.AddonRepository.backgroundUpdateCheck(ids, function BUC_backgroundUpdateCheckCallback() {
          AddonManagerInternal.updateAddonRepositoryData(function BUC_updateAddonCallback() {

            pendingUpdates += aAddons.length;
            aAddons.forEach(function BUC_forEachCallback(aAddon) {
              if (aAddon.id == hotfixID) {
                notifyComplete();
                return;
              }

              
              
              aAddon.findUpdates({
                onUpdateAvailable: function BUC_onUpdateAvailable(aAddon, aInstall) {
                  
                  
                  if (aAddon.permissions & AddonManager.PERM_CAN_UPGRADE &&
                      AddonManager.shouldAutoUpdate(aAddon)) {
                    aInstall.install();
                  }
                },

                onUpdateFinished: notifyComplete
              }, AddonManager.UPDATE_WHEN_PERIODIC_UPDATE);
            });

            notifyComplete();
          });
        });
      });
    }

    if (checkHotfix) {
      var hotfixVersion = "";
      try {
        hotfixVersion = Services.prefs.getCharPref(PREF_EM_HOTFIX_LASTVERSION);
      }
      catch (e) { }

      let url = null;
      if (Services.prefs.getPrefType(PREF_EM_HOTFIX_URL) == Ci.nsIPrefBranch.PREF_STRING)
        url = Services.prefs.getCharPref(PREF_EM_HOTFIX_URL);
      else
        url = Services.prefs.getCharPref(PREF_EM_UPDATE_URL);

      
      url = AddonManager.escapeAddonURI({
        id: hotfixID,
        version: hotfixVersion,
        userDisabled: false,
        appDisabled: false
      }, url);

      pendingUpdates++;
      Components.utils.import("resource://gre/modules/AddonUpdateChecker.jsm");
      AddonUpdateChecker.checkForUpdates(hotfixID, "extension", null, url, {
        onUpdateCheckComplete: function(aUpdates) {
          let update = AddonUpdateChecker.getNewestCompatibleUpdate(aUpdates);
          if (!update) {
            notifyComplete();
            return;
          }

          
          
          if (Services.vc.compare(hotfixVersion, update.version) >= 0) {
            notifyComplete();
            return;
          }

          LOG("Downloading hotfix version " + update.version);
          AddonManager.getInstallForURL(update.updateURL, function(aInstall) {
            aInstall.addListener({
              onDownloadEnded: function(aInstall) {
                try {
                  if (!Services.prefs.getBoolPref(PREF_EM_CERT_CHECKATTRIBUTES))
                    return;
                }
                catch (e) {
                  
                  return;
                }

                try {
                  CertUtils.validateCert(aInstall.certificate,
                                         CertUtils.readCertPrefs(PREF_EM_HOTFIX_CERTS));
                }
                catch (e) {
                  WARN("The hotfix add-on was not signed by the expected " +
                       "certificate and so will not be installed.");
                  aInstall.cancel();
                }
              },

              onInstallEnded: function(aInstall) {
                
                Services.prefs.setCharPref(PREF_EM_HOTFIX_LASTVERSION,
                                           aInstall.version);
              },

              onInstallCancelled: function(aInstall) {
                
                
                Services.prefs.setCharPref(PREF_EM_HOTFIX_LASTVERSION,
                                           hotfixVersion);
              }
            });

            aInstall.install();

            notifyComplete();
          }, "application/x-xpinstall", update.updateHash, null,
             null, update.version);
        },

        onUpdateCheckError: notifyComplete
      });
    }

    notifyComplete();
  },

  











  addStartupChange: function AMI_addStartupChange(aType, aID) {
    if (gStarted)
      return;

    
    for (let type in this.startupChanges)
      this.removeStartupChange(type, aID);

    if (!(aType in this.startupChanges))
      this.startupChanges[aType] = [];
    this.startupChanges[aType].push(aID);
  },

  







  removeStartupChange: function AMI_removeStartupChange(aType, aID) {
    if (gStarted)
      return;

    if (!(aType in this.startupChanges))
      return;

    this.startupChanges[aType] = this.startupChanges[aType].filter(function(aItem) aItem != aID);
  },

  






  callManagerListeners: function AMI_callManagerListeners(aMethod) {
    var args = Array.slice(arguments, 1);
    this.managerListeners.forEach(function(listener) {
      try {
        if (aMethod in listener)
          listener[aMethod].apply(listener, args);
      }
      catch (e) {
        WARN("AddonManagerListener threw exception when calling " + aMethod, e);
      }
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
  
  






  updateAddonRepositoryData: function AMI_updateAddonRepositoryData(aCallback) {
    if (!aCallback)
      throw Components.Exception("Must specify aCallback",
                                 Cr.NS_ERROR_INVALID_ARG);

    new AsyncObjectCaller(this.providers, "updateAddonRepositoryData", {
      nextObject: function(aCaller, aProvider) {
        callProvider(aProvider,
                     "updateAddonRepositoryData",
                     null,
                     aCaller.callNext.bind(aCaller));
      },
      noMoreObjects: function(aCaller) {
        safeCall(aCallback);
      }
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

  








  getAddonBySyncGUID: function AMI_getAddonBySyncGUID(aGUID, aCallback) {
    if (!aGUID || !aCallback) {
      throw Cr.NS_ERROR_INVALID_ARG;
    }

    new AsyncObjectCaller(this.providers, "getAddonBySyncGUID", {
      nextObject: function(aCaller, aProvider) {
        callProvider(aProvider, "getAddonBySyncGUID", null, aGUID, function(aAddon) {
          if (aAddon) {
            safeCall(aCallback, aAddon);
          } else {
            aCaller.callNext();
          }
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

  





  addManagerListener: function AMI_addManagerListener(aListener) {
    if (!this.managerListeners.some(function(i) { return i == aListener; }))
      this.managerListeners.push(aListener);
  },

  





  removeManagerListener: function AMI_removeManagerListener(aListener) {
    let pos = 0;
    while (pos < this.managerListeners.length) {
      if (this.managerListeners[pos] == aListener)
        this.managerListeners.splice(pos, 1);
      else
        pos++;
    }
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

  addTypeListener: function AMI_addTypeListener(aListener) {
    if (!this.typeListeners.some(function(i) { return i == aListener; }))
      this.typeListeners.push(aListener);
  },

  removeTypeListener: function AMI_removeTypeListener(aListener) {
    let pos = 0;
    while (pos < this.typeListeners.length) {
      if (this.typeListeners[pos] == aListener)
        this.typeListeners.splice(pos, 1);
      else
        pos++;
    }
  },

  get addonTypes() {
    return this.typesProxy;
  },

  get autoUpdateDefault() {
    return gAutoUpdateDefault;
  },

  set autoUpdateDefault(aValue) {
    aValue = !!aValue;
    if (aValue != gAutoUpdateDefault)
      Services.prefs.setBoolPref(PREF_EM_AUTOUPDATE_DEFAULT, aValue);
    return aValue;
  },

  get checkCompatibility() {
    return gCheckCompatibility;
  },

  set checkCompatibility(aValue) {
    aValue = !!aValue;
    if (aValue != gCheckCompatibility) {
      if (!aValue)
        Services.prefs.setBoolPref(PREF_EM_CHECK_COMPATIBILITY, false);
      else
        Services.prefs.clearUserPref(PREF_EM_CHECK_COMPATIBILITY);
    }
    return aValue;
  },

  get strictCompatibility() {
    return gStrictCompatibility;
  },

  set strictCompatibility(aValue) {
    aValue = !!aValue;
    if (aValue != gStrictCompatibility)
      Services.prefs.setBoolPref(PREF_EM_STRICT_COMPATIBILITY, aValue);
    return aValue;
  },

  get checkUpdateSecurityDefault() {
    return gCheckUpdateSecurityDefault;
  },

  get checkUpdateSecurity() {
    return gCheckUpdateSecurity;
  },

  set checkUpdateSecurity(aValue) {
    aValue = !!aValue;
    if (aValue != gCheckUpdateSecurity) {
      if (aValue != gCheckUpdateSecurityDefault)
        Services.prefs.setBoolPref(PREF_EM_CHECK_UPDATE_SECURITY, aValue);
      else
        Services.prefs.clearUserPref(PREF_EM_CHECK_UPDATE_SECURITY);
    }
    return aValue;
  },

  get updateEnabled() {
    return gUpdateEnabled;
  },

  set updateEnabled(aValue) {
    aValue = !!aValue;
    if (aValue != gUpdateEnabled)
      Services.prefs.setBoolPref(PREF_EM_UPDATE_ENABLED, aValue);
    return aValue;
  }
};







var AddonManagerPrivate = {
  startup: function AMP_startup() {
    AddonManagerInternal.startup();
  },

  registerProvider: function AMP_registerProvider(aProvider, aTypes) {
    AddonManagerInternal.registerProvider(aProvider, aTypes);
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

  addStartupChange: function AMP_addStartupChange(aType, aID) {
    AddonManagerInternal.addStartupChange(aType, aID);
  },

  removeStartupChange: function AMP_removeStartupChange(aType, aID) {
    AddonManagerInternal.removeStartupChange(aType, aID);
  },

  notifyAddonChanged: function AMP_notifyAddonChanged(aId, aType, aPendingRestart) {
    AddonManagerInternal.notifyAddonChanged(aId, aType, aPendingRestart);
  },

  updateAddonAppDisabledStates: function AMP_updateAddonAppDisabledStates() {
    AddonManagerInternal.updateAddonAppDisabledStates();
  },

  updateAddonRepositoryData: function AMP_updateAddonRepositoryData(aCallback) {
    AddonManagerInternal.updateAddonRepositoryData(aCallback);
  },

  callInstallListeners: function AMP_callInstallListeners(aMethod) {
    return AddonManagerInternal.callInstallListeners.apply(AddonManagerInternal,
                                                           arguments);
  },

  callAddonListeners: function AMP_callAddonListeners(aMethod) {
    AddonManagerInternal.callAddonListeners.apply(AddonManagerInternal, arguments);
  },

  AddonAuthor: AddonAuthor,

  AddonScreenshot: AddonScreenshot,

  AddonCompatibilityOverride: AddonCompatibilityOverride,

  AddonType: AddonType
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

  
  VIEW_TYPE_LIST: "list",

  TYPE_UI_HIDE_EMPTY: 16,

  
  
  AUTOUPDATE_DISABLE: 0,
  
  
  AUTOUPDATE_DEFAULT: 1,
  
  AUTOUPDATE_ENABLE: 2,

  
  
  OPTIONS_TYPE_DIALOG: 1,
  
  OPTIONS_TYPE_INLINE: 2,
  
  OPTIONS_TYPE_TAB: 3,

  
  
  
  STARTUP_CHANGE_INSTALLED: "installed",
  
  
  
  STARTUP_CHANGE_CHANGED: "changed",
  
  
  STARTUP_CHANGE_UNINSTALLED: "uninstalled",
  
  
  
  STARTUP_CHANGE_DISABLED: "disabled",
  
  
  
  STARTUP_CHANGE_ENABLED: "enabled",

  getInstallForURL: function AM_getInstallForURL(aUrl, aCallback, aMimetype,
                                                 aHash, aName, aIconURL,
                                                 aVersion, aLoadGroup) {
    AddonManagerInternal.getInstallForURL(aUrl, aCallback, aMimetype, aHash,
                                          aName, aIconURL, aVersion, aLoadGroup);
  },

  getInstallForFile: function AM_getInstallForFile(aFile, aCallback, aMimetype) {
    AddonManagerInternal.getInstallForFile(aFile, aCallback, aMimetype);
  },

  






  getStartupChanges: function AM_getStartupChanges(aType) {
    if (!(aType in AddonManagerInternal.startupChanges))
      return [];
    return AddonManagerInternal.startupChanges[aType].slice(0);
  },

  getAddonByID: function AM_getAddonByID(aId, aCallback) {
    AddonManagerInternal.getAddonByID(aId, aCallback);
  },

  getAddonBySyncGUID: function AM_getAddonBySyncGUID(aId, aCallback) {
    AddonManagerInternal.getAddonBySyncGUID(aId, aCallback);
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

  addManagerListener: function AM_addManagerListener(aListener) {
    AddonManagerInternal.addManagerListener(aListener);
  },

  removeManagerListener: function AM_removeManagerListener(aListener) {
    AddonManagerInternal.removeManagerListener(aListener);
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

  addTypeListener: function AM_addTypeListener(aListener) {
    AddonManagerInternal.addTypeListener(aListener);
  },

  removeTypeListener: function AM_removeTypeListener(aListener) {
    AddonManagerInternal.removeTypeListener(aListener);
  },

  get addonTypes() {
    return AddonManagerInternal.addonTypes;
  },

  shouldAutoUpdate: function AM_shouldAutoUpdate(aAddon) {
    if (!("applyBackgroundUpdates" in aAddon))
      return false;
    if (aAddon.applyBackgroundUpdates == AddonManager.AUTOUPDATE_ENABLE)
      return true;
    if (aAddon.applyBackgroundUpdates == AddonManager.AUTOUPDATE_DISABLE)
      return false;
    return this.autoUpdateDefault;
  },

  get checkCompatibility() {
    return AddonManagerInternal.checkCompatibility;
  },

  set checkCompatibility(aValue) {
    AddonManagerInternal.checkCompatibility = aValue;
  },

  get strictCompatibility() {
    return AddonManagerInternal.strictCompatibility;
  },

  set strictCompatibility(aValue) {
    AddonManagerInternal.strictCompatibility = aValue;
  },

  get checkUpdateSecurityDefault() {
    return AddonManagerInternal.checkUpdateSecurityDefault;
  },

  get checkUpdateSecurity() {
    return AddonManagerInternal.checkUpdateSecurity;
  },

  set checkUpdateSecurity(aValue) {
    AddonManagerInternal.checkUpdateSecurity = aValue;
  },

  get updateEnabled() {
    return AddonManagerInternal.updateEnabled;
  },

  set updateEnabled(aValue) {
    AddonManagerInternal.updateEnabled = aValue;
  },

  get autoUpdateDefault() {
    return AddonManagerInternal.autoUpdateDefault;
  },

  set autoUpdateDefault(aValue) {
    AddonManagerInternal.autoUpdateDefault = aValue;
  },

  escapeAddonURI: function AM_escapeAddonURI(aAddon, aUri, aAppVersion) {
    return AddonManagerInternal.escapeAddonURI(aAddon, aUri, aAppVersion);
  }
};

Object.freeze(AddonManagerInternal);
Object.freeze(AddonManagerPrivate);
Object.freeze(AddonManager);
