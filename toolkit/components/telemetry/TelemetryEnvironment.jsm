



"use strict";

this.EXPORTED_SYMBOLS = [
  "TelemetryEnvironment",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/AddonManager.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/PromiseUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "ctypes",
                                  "resource://gre/modules/ctypes.jsm");
#ifndef MOZ_WIDGET_GONK
XPCOMUtils.defineLazyModuleGetter(this, "LightweightThemeManager",
                                  "resource://gre/modules/LightweightThemeManager.jsm");
#endif
XPCOMUtils.defineLazyModuleGetter(this, "ProfileAge",
                                  "resource://gre/modules/ProfileAge.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
                                  "resource://gre/modules/UpdateChannel.jsm");

const LOGGER_NAME = "Toolkit.Telemetry";

const PREF_BLOCKLIST_ENABLED = "extensions.blocklist.enabled";
const PREF_DISTRIBUTION_ID = "distribution.id";
const PREF_DISTRIBUTION_VERSION = "distribution.version";
const PREF_DISTRIBUTOR = "app.distributor";
const PREF_DISTRIBUTOR_CHANNEL = "app.distributor.channel";
const PREF_E10S_ENABLED = "browser.tabs.remote.autostart";
const PREF_HOTFIX_LASTVERSION = "extensions.hotfix.lastVersion";
const PREF_APP_PARTNER_BRANCH = "app.partner.";
const PREF_PARTNER_ID = "mozilla.partner.id";
const PREF_TELEMETRY_ENABLED = "toolkit.telemetry.enabled";
const PREF_UPDATE_ENABLED = "app.update.enabled";
const PREF_UPDATE_AUTODOWNLOAD = "app.update.auto";

const MILLISECONDS_PER_DAY = 24 * 60 * 60 * 1000;

const EXPERIMENTS_CHANGED_TOPIC = "experiments-changed";







function truncateToDays(aMsec) {
  return Math.floor(aMsec / MILLISECONDS_PER_DAY);
}





function getBrowserLocale() {
  try {
    return Cc["@mozilla.org/chrome/chrome-registry;1"].
             getService(Ci.nsIXULChromeRegistry).
             getSelectedLocale('global');
  } catch (e) {
    return null;
  }
}





function getSystemLocale() {
  try {
    return Services.locale.getLocaleComponentForUserAgent();
  } catch (e) {
    return null;
  }
}







function promiseGetAddonsByTypes(aTypes) {
  return new Promise((resolve) =>
                     AddonManager.getAddonsByTypes(aTypes, (addons) => resolve(addons)));
}









function getSysinfoProperty(aPropertyName, aDefault) {
  try {
    
    return Services.sysinfo.getProperty(aPropertyName);
  } catch (e) {}

  return aDefault;
}









function getGfxField(aPropertyName, aDefault) {
  let gfxInfo = Cc["@mozilla.org/gfx/info;1"].getService(Ci.nsIGfxInfo);

  try {
    
    let gfxProp = gfxInfo[aPropertyName];
    if (gfxProp !== "") {
      return gfxProp;
    }
  } catch (e) {}

  return aDefault;
}







function getGfxAdapter(aSuffix = "") {
  let memoryMB = getGfxField("adapterRAM" + aSuffix, null);
  if (memoryMB) {
    memoryMB = parseInt(memoryMB, 10);
  }

  return {
    description: getGfxField("adapterDescription" + aSuffix, null),
    vendorID: getGfxField("adapterVendorID" + aSuffix, null),
    deviceID: getGfxField("adapterDeviceID" + aSuffix, null),
    subsysID: getGfxField("adapterSubsysID" + aSuffix, null),
    RAM: memoryMB,
    driver: getGfxField("adapterDriver" + aSuffix, null),
    driverVersion: getGfxField("adapterDriverVersion" + aSuffix, null),
    driverDate: getGfxField("adapterDriverDate" + aSuffix, null),
  };
}

#ifdef XP_WIN






function getServicePack() {
  const BYTE = ctypes.uint8_t;
  const WORD = ctypes.uint16_t;
  const DWORD = ctypes.uint32_t;
  const WCHAR = ctypes.char16_t;
  const BOOL = ctypes.int;

  
  
  const SZCSDVERSIONLENGTH = 128;
  const OSVERSIONINFOEXW = new ctypes.StructType('OSVERSIONINFOEXW',
      [
      {dwOSVersionInfoSize: DWORD},
      {dwMajorVersion: DWORD},
      {dwMinorVersion: DWORD},
      {dwBuildNumber: DWORD},
      {dwPlatformId: DWORD},
      {szCSDVersion: ctypes.ArrayType(WCHAR, SZCSDVERSIONLENGTH)},
      {wServicePackMajor: WORD},
      {wServicePackMinor: WORD},
      {wSuiteMask: WORD},
      {wProductType: BYTE},
      {wReserved: BYTE}
      ]);

  let kernel32 = ctypes.open("kernel32");
  try {
    let GetVersionEx = kernel32.declare("GetVersionExW",
                                        ctypes.default_abi,
                                        BOOL,
                                        OSVERSIONINFOEXW.ptr);
    let winVer = OSVERSIONINFOEXW();
    winVer.dwOSVersionInfoSize = OSVERSIONINFOEXW.size;

    if(0 === GetVersionEx(winVer.address())) {
      throw("Failure in GetVersionEx (returned 0)");
    }

    return {
      major: winVer.wServicePackMajor,
      minor: winVer.wServicePackMinor,
    };
  } catch (e) {
    return {
      major: null,
      minor: null,
    };
  } finally {
    kernel32.close();
  }
}
#endif

this.TelemetryEnvironment = {
  _shutdown: true,

  
  _changeListeners: new Map(),
  
  _collectTask: null,

  
  RECORD_PREF_STATE: 1, 
  RECORD_PREF_VALUE: 2, 
  RECORD_PREF_NOTIFY_ONLY: 3, 

  
  
  _watchedPrefs: null,

  
  _addonsListener: null,

  
  
  
  
  _cachedAddons: null,

  


  init: function () {
    if (!this._shutdown) {
      this._log.error("init - Already initialized");
      return;
    }

    this._configureLog();
    this._log.trace("init");
    this._shutdown = false;
    this._startWatchingPrefs();
    this._startWatchingAddons();

    AddonManager.shutdown.addBlocker("TelemetryEnvironment: caching addons",
                                      () => this._blockAddonManagerShutdown(),
                                      () => this._getState());
  },

  



  shutdown: Task.async(function* () {
    if (this._shutdown) {
      if (this._log) {
        this._log.error("shutdown - Already shut down");
      } else {
        Cu.reportError("TelemetryEnvironment.shutdown - Already shut down");
      }
      return;
    }

    this._log.trace("shutdown");
    this._shutdown = true;
    this._stopWatchingPrefs();
    this._stopWatchingAddons();
    this._changeListeners.clear();
    yield this._collectTask;

    this._cachedAddons = null;
  }),

  _configureLog: function () {
    if (this._log) {
      return;
    }
    this._log = Log.repository.getLoggerWithMessagePrefix(
                                 LOGGER_NAME, "TelemetryEnvironment::");
  },

  





  registerChangeListener: function (name, listener) {
    this._configureLog();
    this._log.trace("registerChangeListener for " + name);
    if (this._shutdown) {
      this._log.warn("registerChangeListener - already shutdown")
      return;
    }
    this._changeListeners.set(name, listener);
  },

  




  unregisterChangeListener: function (name) {
    this._configureLog();
    this._log.trace("unregisterChangeListener for " + name);
    if (this._shutdown) {
      this._log.warn("registerChangeListener - already shutdown")
      return;
    }
    this._changeListeners.delete(name);
  },

  



  _watchPreferences: function (aPreferences) {
    if (this._watchedPrefs) {
      this._stopWatchingPrefs();
    }

    this._watchedPrefs = aPreferences;
    this._startWatchingPrefs();
  },

  





  _getPrefData: function () {
    if (!this._watchedPrefs) {
      return {};
    }

    let prefData = {};
    for (let pref in this._watchedPrefs) {
      
      if (!Preferences.isSet(pref) ||
          this._watchedPrefs[pref] == this.RECORD_PREF_NOTIFY_ONLY) {
        continue;
      }

      
      
      let prefValue = undefined;
      if (this._watchedPrefs[pref] == this.RECORD_PREF_STATE) {
        prefValue = null;
      } else {
        prefValue = Preferences.get(pref, null);
      }
      prefData[pref] = prefValue;
    }
    return prefData;
  },

  


  _startWatchingPrefs: function () {
    this._log.trace("_startWatchingPrefs - " + this._watchedPrefs);

    if (!this._watchedPrefs) {
      return;
    }

    for (let pref in this._watchedPrefs) {
      Preferences.observe(pref, this._onPrefChanged, this);
    }
  },

  


  _stopWatchingPrefs: function () {
    this._log.trace("_stopWatchingPrefs");

    if (!this._watchedPrefs) {
      return;
    }

    for (let pref in this._watchedPrefs) {
      Preferences.ignore(pref, this._onPrefChanged, this);
    }

    this._watchedPrefs = null;
  },

  _onPrefChanged: function () {
    this._log.trace("_onPrefChanged");
    this._onEnvironmentChange("pref-changed");
  },

  



  _getBuild: function () {
    let buildData = {
      applicationId: Services.appinfo.ID,
      applicationName: Services.appinfo.name,
      architecture: Services.sysinfo.get("arch"),
      buildId: Services.appinfo.appBuildID,
      version: Services.appinfo.version,
      vendor: Services.appinfo.vendor,
      platformVersion: Services.appinfo.platformVersion,
      xpcomAbi: Services.appinfo.XPCOMABI,
      hotfixVersion: Preferences.get(PREF_HOTFIX_LASTVERSION, null),
    };

    
    if ("@mozilla.org/xpcom/mac-utils;1" in Cc) {
      let macUtils = Cc["@mozilla.org/xpcom/mac-utils;1"].getService(Ci.nsIMacUtils);
      if (macUtils && macUtils.isUniversalBinary) {
        buildData.architecturesInBinary = macUtils.architecturesInBinary;
      }
    }

    return buildData;
  },

  



  _isDefaultBrowser: function () {
    if (!("@mozilla.org/browser/shell-service;1" in Cc)) {
      this._log.error("_isDefaultBrowser - Could not obtain shell service");
      return null;
    }

    let shellService;
    try {
      shellService = Cc["@mozilla.org/browser/shell-service;1"]
                       .getService(Ci.nsIShellService);
    } catch (ex) {
      this._log.error("_isDefaultBrowser - Could not obtain shell service", ex);
      return null;
    }

    if (shellService) {
      try {
        
        return shellService.isDefaultBrowser(false, true) ? true : false;
      } catch (ex) {
        this._log.error("_isDefaultBrowser - Could not determine if default browser", ex);
        return null;
      }
    }

    return null;
  },

  



  _getSettings: function () {
    let updateChannel = null;
    try {
      updateChannel = UpdateChannel.get();
    } catch (e) {}

    return {
      blocklistEnabled: Preferences.get(PREF_BLOCKLIST_ENABLED, true),
#ifndef MOZ_WIDGET_ANDROID
      isDefaultBrowser: this._isDefaultBrowser(),
#endif
      e10sEnabled: Preferences.get(PREF_E10S_ENABLED, false),
      telemetryEnabled: Preferences.get(PREF_TELEMETRY_ENABLED, false),
      locale: getBrowserLocale(),
      update: {
        channel: updateChannel,
        enabled: Preferences.get(PREF_UPDATE_ENABLED, true),
        autoDownload: Preferences.get(PREF_UPDATE_AUTODOWNLOAD, true),
      },
      userPrefs: this._getPrefData(),
    };
  },

  



  _getProfile: Task.async(function* () {
    let profileAccessor = new ProfileAge(null, this._log);

    let creationDate = yield profileAccessor.created;
    let resetDate = yield profileAccessor.reset;

    let profileData = {
      creationDate: truncateToDays(creationDate),
    };

    if (resetDate) {
      profileData.resetDate = truncateToDays(resetDate);
    }
    return profileData;
  }),

  



  _getPartner: function () {
    let partnerData = {
      distributionId: Preferences.get(PREF_DISTRIBUTION_ID, null),
      distributionVersion: Preferences.get(PREF_DISTRIBUTION_VERSION, null),
      partnerId: Preferences.get(PREF_PARTNER_ID, null),
      distributor: Preferences.get(PREF_DISTRIBUTOR, null),
      distributorChannel: Preferences.get(PREF_DISTRIBUTOR_CHANNEL, null),
    };

    
    let partnerBranch = Services.prefs.getBranch(PREF_APP_PARTNER_BRANCH);
    partnerData.partnerNames = partnerBranch.getChildList("")

    return partnerData;
  },

  



  _getCpuData: function () {
    let cpuData = {
      count: getSysinfoProperty("cpucount", null),
      vendor: null, 
      family: null, 
      model: null, 
      stepping: null, 
    };

    const CPU_EXTENSIONS = ["hasMMX", "hasSSE", "hasSSE2", "hasSSE3", "hasSSSE3",
                            "hasSSE4A", "hasSSE4_1", "hasSSE4_2", "hasEDSP", "hasARMv6",
                            "hasARMv7", "hasNEON"];

    
    let availableExts = [];
    for (let ext of CPU_EXTENSIONS) {
      try {
        Services.sysinfo.getProperty(ext);
        
        availableExts.push(ext);
      } catch (e) {}
    }

    cpuData.extensions = availableExts;

    return cpuData;
  },

#if defined(MOZ_WIDGET_GONK) || defined(MOZ_WIDGET_ANDROID)
  



  _getDeviceData: function () {
    return {
      model: getSysinfoProperty("device", null),
      manufacturer: getSysinfoProperty("manufacturer", null),
      hardware: getSysinfoProperty("hardware", null),
      isTablet: getSysinfoProperty("tablet", null),
    };
  },
#endif

  



  _getOSData: function () {
#ifdef XP_WIN
    
    let servicePack = getServicePack();
#endif

    return {
      name: getSysinfoProperty("name", null),
      version: getSysinfoProperty("version", null),
#if defined(MOZ_WIDGET_GONK) || defined(MOZ_WIDGET_ANDROID)
      kernelVersion: getSysinfoProperty("kernel_version", null),
#elif defined(XP_WIN)
      servicePackMajor: servicePack.major,
      servicePackMinor: servicePack.minor,
#endif
      locale: getSystemLocale(),
    };
  },

  



  _getHDDData: function () {
    return {
      profile: { 
        model: getSysinfoProperty("profileHDDModel", null),
        revision: getSysinfoProperty("profileHDDRevision", null),
      },
      binary:  { 
        model: getSysinfoProperty("binHDDModel", null),
        revision: getSysinfoProperty("binHDDRevision", null),
      },
      system:  { 
        model: getSysinfoProperty("winHDDModel", null),
        revision: getSysinfoProperty("winHDDRevision", null),
      },
    };
  },

  



  _getGFXData: function () {
    let gfxData = {
      D2DEnabled: getGfxField("D2DEnabled", null),
      DWriteEnabled: getGfxField("DWriteEnabled", null),
      DWriteVersion: getGfxField("DWriteVersion", null),
      adapters: [],
    };

    
    gfxData.adapters.push(getGfxAdapter(""));
    gfxData.adapters[0].GPUActive = true;

    
    let hasGPU2 = getGfxField("adapterDeviceID2", null) !== null;
    if (!hasGPU2) {
      this._log.trace("_getGFXData - Only one display adapter detected.");
      return gfxData;
    }

    this._log.trace("_getGFXData - Two display adapters detected.");

    gfxData.adapters.push(getGfxAdapter("2"));
    gfxData.adapters[1].GPUActive = getGfxField("isGPU2Active ", null);

    return gfxData;
  },

  



  _getSystem: function () {
    let memoryMB = getSysinfoProperty("memsize", null);
    if (memoryMB) {
      
      
      memoryMB = Math.round(memoryMB / 1024 / 1024);
    }

    return {
      memoryMB: memoryMB,
#ifdef XP_WIN
      isWow64: getSysinfoProperty("isWow64", null),
#endif
      cpu: this._getCpuData(),
#if defined(MOZ_WIDGET_GONK) || defined(MOZ_WIDGET_ANDROID)
      device: this._getDeviceData(),
#endif
      os: this._getOSData(),
      hdd: this._getHDDData(),
      gfx: this._getGFXData(),
    };
  },

  







  _getActiveAddons: Task.async(function* () {

    
    let allAddons = yield promiseGetAddonsByTypes(["extension", "service"]);

    let activeAddons = {};
    for (let addon of allAddons) {
      
      if (!addon.isActive) {
        continue;
      }

      
      let installDate = new Date(Math.max(0, addon.installDate));
      let updateDate = new Date(Math.max(0, addon.updateDate));

      activeAddons[addon.id] = {
        blocklisted: (addon.blocklistState !== Ci.nsIBlocklistService.STATE_NOT_BLOCKED),
        description: addon.description,
        name: addon.name,
        userDisabled: addon.userDisabled,
        appDisabled: addon.appDisabled,
        version: addon.version,
        scope: addon.scope,
        type: addon.type,
        foreignInstall: addon.foreignInstall,
        hasBinaryComponents: addon.hasBinaryComponents,
        installDay: truncateToDays(installDate.getTime()),
        updateDay: truncateToDays(updateDate.getTime()),
      };
    }

    return activeAddons;
  }),

  







  _getActiveTheme: Task.async(function* () {
    
    let themes = yield promiseGetAddonsByTypes(["theme"]);

    let activeTheme = {};
    
    let theme = themes.find(theme => theme.isActive);
    if (theme) {
      
      let installDate = new Date(Math.max(0, theme.installDate));
      let updateDate = new Date(Math.max(0, theme.updateDate));

      activeTheme = {
        id: theme.id,
        blocklisted: (theme.blocklistState !== Ci.nsIBlocklistService.STATE_NOT_BLOCKED),
        description: theme.description,
        name: theme.name,
        userDisabled: theme.userDisabled,
        appDisabled: theme.appDisabled,
        version: theme.version,
        scope: theme.scope,
        foreignInstall: theme.foreignInstall,
        hasBinaryComponents: theme.hasBinaryComponents,
        installDay: truncateToDays(installDate.getTime()),
        updateDay: truncateToDays(updateDate.getTime()),
      };
    }

    return activeTheme;
  }),

  



  _getActivePlugins: function () {
    let pluginTags =
      Cc["@mozilla.org/plugin/host;1"].getService(Ci.nsIPluginHost).getPluginTags({});

    let activePlugins = [];
    for (let tag of pluginTags) {
      
      if (tag.disabled) {
        continue;
      }

      
      let updateDate = new Date(Math.max(0, tag.lastModifiedTime));

      activePlugins.push({
        name: tag.name,
        version: tag.version,
        description: tag.description,
        blocklisted: tag.blocklisted,
        disabled: tag.disabled,
        clicktoplay: tag.clicktoplay,
        mimeTypes: tag.getMimeTypes({}),
        updateDay: truncateToDays(updateDate.getTime()),
      });
    }

    return activePlugins;
  },

  







  _getActiveGMPlugins: Task.async(function* () {
    
    let allPlugins = yield promiseGetAddonsByTypes(["plugin"]);

    let activeGMPlugins = {};
    for (let plugin of allPlugins) {
      
      if (!plugin.isGMPlugin) {
        continue;
      }

      activeGMPlugins[plugin.id] = {
        version: plugin.version,
        userDisabled: plugin.userDisabled,
        applyBackgroundUpdates: plugin.applyBackgroundUpdates,
      };
    }

    return activeGMPlugins;
  }),

  



  _getActiveExperiment: function () {
    let experimentInfo = {};
    try {
      let scope = {};
      Cu.import("resource:///modules/experiments/Experiments.jsm", scope);
      let experiments = scope.Experiments.instance()
      let activeExperiment = experiments.getActiveExperimentID();
      if (activeExperiment) {
        experimentInfo.id = activeExperiment;
        experimentInfo.branch = experiments.getActiveExperimentBranch();
      }
    } catch(e) {
      
      return experimentInfo;
    }

    return experimentInfo;
  },

  







  _getAddons: Task.async(function* () {
    
    
    
    let addons = this._cachedAddons || {};
    if (!this._cachedAddons) {
      addons.activeAddons = yield this._getActiveAddons();
      addons.activeTheme = yield this._getActiveTheme();
      addons.activeGMPlugins = yield this._getActiveGMPlugins();
    }

    let personaId = null;
#ifndef MOZ_WIDGET_GONK
    let theme = LightweightThemeManager.currentTheme;
    if (theme) {
      personaId = theme.id;
    }
#endif

    let addonData = {
      activeAddons: addons.activeAddons,
      theme: addons.activeTheme,
      activePlugins: this._getActivePlugins(),
      activeGMPlugins: addons.activeGMPlugins,
      activeExperiment: this._getActiveExperiment(),
      persona: personaId,
    };

    return addonData;
  }),

  


  _startWatchingAddons: function () {
    
    
    
    
    
    
    
    
    
    
    
    

    this._addonsListener = {
      onEnabled: addon => {
        this._log.trace("_addonsListener - onEnabled " + addon.id);
        this._onActiveAddonsChanged(addon)
      },
      onDisabled: addon => {
        this._log.trace("_addonsListener - onDisabled " + addon.id);
        this._onActiveAddonsChanged(addon);
      },
      onInstalled: addon => {
        this._log.trace("_addonsListener - onInstalled " + addon.id +
                        ", isActive: " + addon.isActive);
        if (addon.isActive) {
          this._onActiveAddonsChanged(addon);
        }
      },
      onUninstalling: (addon, requiresRestart) => {
        this._log.trace("_addonsListener - onUninstalling " + addon.id +
                        ", isActive: " + addon.isActive +
                        ", requiresRestart: " + requiresRestart);
        if (!addon.isActive || requiresRestart) {
          return;
        }
        this._onActiveAddonsChanged(addon);
      },
    };

    AddonManager.addAddonListener(this._addonsListener);

    
    Services.obs.addObserver(this, EXPERIMENTS_CHANGED_TOPIC, false);
  },

  


  _stopWatchingAddons: function () {
    if (this._addonsListener) {
      AddonManager.removeAddonListener(this._addonsListener);
      Services.obs.removeObserver(this, EXPERIMENTS_CHANGED_TOPIC);
    }
    this._addonsListener = null;
  },

  



  _onActiveAddonsChanged: function (aAddon) {
    const INTERESTING_ADDONS = [ "extension", "plugin", "service", "theme" ];

    this._log.trace("_onActiveAddonsChanged - id " + aAddon.id + ", type " + aAddon.type);

    if (INTERESTING_ADDONS.find(addon => addon == aAddon.type)) {
      this._onEnvironmentChange("addons-changed");
    }
  },

  


  observe: function (aSubject, aTopic, aData) {
    this._log.trace("observe - Topic " + aTopic);

    if (aTopic == EXPERIMENTS_CHANGED_TOPIC) {
      this._onEnvironmentChange("experiment-changed");
    }
  },

  



  getEnvironmentData: function() {
    if (this._shutdown) {
      this._log.error("getEnvironmentData - Already shut down");
      return Promise.reject("Already shutdown");
    }

    this._log.trace("getEnvironmentData");
    if (this._collectTask) {
      return this._collectTask;
    }

    this._collectTask = this._doGetEnvironmentData();
    let clear = () => this._collectTask = null;
    this._collectTask.then(clear, clear);
    return this._collectTask;
  },

  _doGetEnvironmentData: Task.async(function* () {
    this._log.trace("getEnvironmentData");

    
    let sections = {
      "build" : () => this._getBuild(),
      "settings": () => this._getSettings(),
#ifndef MOZ_WIDGET_ANDROID
      "profile": () => this._getProfile(),
#endif
      "partner": () => this._getPartner(),
      "system": () => this._getSystem(),
      "addons": () => this._getAddons(),
    };

    let data = {};
    
    
    
    for (let s in sections) {
      try {
        data[s] = yield sections[s]();
      } catch (e) {
        this._log.error("_doGetEnvironmentData - There was an exception collecting " + s, e);
      }
    }

    return data;
  }),

  _onEnvironmentChange: function (what) {
    this._log.trace("_onEnvironmentChange for " + what);
    if (this._shutdown) {
      this._log.trace("_onEnvironmentChange - Already shut down.");
      return;
    }

    for (let [name, listener] of this._changeListeners) {
      try {
        this._log.debug("_onEnvironmentChange - calling " + name);
        listener();
      } catch (e) {
        this._log.warning("_onEnvironmentChange - listener " + name + " caught error", e);
      }
    }
  },

  



  _blockAddonManagerShutdown: Task.async(function*() {
    this._log.trace("_blockAddonManagerShutdown");

    this._stopWatchingAddons();

    this._cachedAddons = {
      activeAddons: yield this._getActiveAddons(),
      activeTheme: yield this._getActiveTheme(),
      activeGMPlugins: yield this._getActiveGMPlugins(),
    };

    yield this._collectTask;
  }),

  


  _getState: function() {
    return {
      shutdown: this._shutdown,
      hasCollectTask: !!this._collectTask,
      hasAddonsListener: !!this._addonsListener,
      hasCachedAddons: !!this._cachedAddons,
    };
  },
};
