



"use strict";

this.EXPORTED_SYMBOLS = [
  "TelemetryEnvironment",
];

const {classes: Cc, interfaces: Ci, utils: Cu} = Components;
const myScope = this;

Cu.import("resource://gre/modules/AddonManager.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/PromiseUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/ObjectUtils.jsm");

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

const CHANGE_THROTTLE_INTERVAL_MS = 5 * 60 * 1000;




let Policy = {
  now: () => new Date(),
};

var gGlobalEnvironment;
function getGlobal() {
  if (!gGlobalEnvironment) {
    gGlobalEnvironment = new EnvironmentCache();
  }
  return gGlobalEnvironment;
}

const TelemetryEnvironment = {
  get currentEnvironment() {
    return getGlobal().currentEnvironment;
  },

  onInitialized: function() {
    return getGlobal().onInitialized();
  },

  registerChangeListener: function(name, listener) {
    return getGlobal().registerChangeListener(name, listener);
  },

  unregisterChangeListener: function(name) {
    return getGlobal().unregisterChangeListener(name);
  },

  
  RECORD_PREF_STATE: 1, 
  RECORD_PREF_VALUE: 2, 
  RECORD_PREF_NOTIFY_ONLY: 3, 

  
  _watchPreferences: function(prefMap) {
    return getGlobal()._watchPreferences(prefMap);
  },
};

const DEFAULT_ENVIRONMENT_PREFS = new Map([
  ["app.feedback.baseURL", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["app.support.baseURL", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["accessibility.browsewithcaret", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["accessibility.force_disabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["app.update.auto", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["app.update.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["app.update.interval", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["app.update.service.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["app.update.silent", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["app.update.url", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["browser.cache.disk.enable", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["browser.cache.disk.capacity", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["browser.cache.memory.enable", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["browser.cache.offline.enable", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["browser.formfill.enable", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["browser.newtab.url", TelemetryEnvironment.RECORD_PREF_STATE],
  ["browser.newtabpage.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["browser.newtabpage.enhanced", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["browser.polaris.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["browser.shell.checkDefaultBrowser", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["browser.startup.homepage", TelemetryEnvironment.RECORD_PREF_STATE],
  ["browser.startup.page", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["devtools.chrome.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["devtools.debugger.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["devtools.debugger.remote-enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["dom.ipc.plugins.asyncInit", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["dom.ipc.plugins.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["experiments.manifest.uri", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["extensions.blocklist.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["extensions.blocklist.url", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["extensions.strictCompatibility", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["extensions.update.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["extensions.update.url", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["extensions.update.background.url", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["general.smoothScroll", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["gfx.direct2d.disabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["gfx.direct2d.force-enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["gfx.direct2d.use1_1", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["layers.acceleration.disabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["layers.acceleration.force-enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["layers.async-pan-zoom.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["layers.async-video-oop.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["layers.async-video.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["layers.componentalpha.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["layers.d3d11.disable-warp", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["layers.d3d11.force-warp", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["layers.prefer-d3d9", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["layers.prefer-opengl", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["layout.css.devPixelsPerPx", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["network.proxy.autoconfig_url", TelemetryEnvironment.RECORD_PREF_STATE],
  ["network.proxy.http", TelemetryEnvironment.RECORD_PREF_STATE],
  ["network.proxy.ssl", TelemetryEnvironment.RECORD_PREF_STATE],
  ["pdfjs.disabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["places.history.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["privacy.trackingprotection.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["privacy.donottrackheader.enabled", TelemetryEnvironment.RECORD_PREF_VALUE],
  ["services.sync.serverURL", TelemetryEnvironment.RECORD_PREF_STATE],
]);

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
  
  
  let memoryMB = parseInt(getGfxField("adapterRAM" + aSuffix, null), 10);
  if (Number.isNaN(memoryMB)) {
    memoryMB = null;
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





function EnvironmentAddonBuilder(environment) {
  this._environment = environment;

  
  
  this._pendingTask = null;

  
  this._loaded = false;
}
EnvironmentAddonBuilder.prototype = {
  



  init: function() {
    
    
    try {
      AddonManager.shutdown.addBlocker("EnvironmentAddonBuilder",
        () => this._shutdownBlocker());
    } catch (err) {
      return Promise.reject(err);
    }

    this._pendingTask = this._updateAddons().then(
      () => { this._pendingTask = null; },
      (err) => {
        this._environment._log.error("init - Exception in _updateAddons", err);
        this._pendingTask = null;
      }
    );

    return this._pendingTask;
  },

  


  watchForChanges: function() {
    this._loaded = true;
    AddonManager.addAddonListener(this);
    Services.obs.addObserver(this, EXPERIMENTS_CHANGED_TOPIC, false);
  },

  
  onEnabled: function() {
    this._onAddonChange();
  },
  onDisabled: function() {
    this._onAddonChange();
  },
  onInstalled: function() {
    this._onAddonChange();
  },
  onUninstalling: function() {
    this._onAddonChange();
  },

  _onAddonChange: function() {
    this._environment._log.trace("_onAddonChange");
    this._checkForChanges("addons-changed");
  },

  
  observe: function (aSubject, aTopic, aData) {
    this._environment._log.trace("observe - Topic " + aTopic);
    this._checkForChanges("experiment-changed");
  },

  _checkForChanges: function(changeReason) {
    if (this._pendingTask) {
      this._environment._log.trace("_checkForChanges - task already pending, dropping change with reason " + changeReason);
      return;
    }

    this._pendingTask = this._updateAddons().then(
      (result) => {
        this._pendingTask = null;
        if (result.changed) {
          this._environment._onEnvironmentChange(changeReason, result.oldEnvironment);
        }
      },
      (err) => {
        this._pendingTask = null;
        this._environment._log.error("_checkForChanges: Error collecting addons", err);
      });
  },

  _shutdownBlocker: function() {
    if (this._loaded) {
      AddonManager.removeAddonListener(this);
      Services.obs.removeObserver(this, EXPERIMENTS_CHANGED_TOPIC);
    }
    return this._pendingTask;
  },

  









  _updateAddons: Task.async(function* () {
    this._environment._log.trace("_updateAddons");
    let personaId = null;
#ifndef MOZ_WIDGET_GONK
    let theme = LightweightThemeManager.currentTheme;
    if (theme) {
      personaId = theme.id;
    }
#endif

    let addons = {
      activeAddons: yield this._getActiveAddons(),
      theme: yield this._getActiveTheme(),
      activePlugins: this._getActivePlugins(),
      activeGMPlugins: yield this._getActiveGMPlugins(),
      activeExperiment: this._getActiveExperiment(),
      persona: personaId,
    };

    let result = {
      changed: !ObjectUtils.deepEqual(addons, this._environment._currentEnvironment.addons),
    };

    if (result.changed) {
      this._environment._log.trace("_updateAddons: addons differ");
      result.oldEnvironment = Cu.cloneInto(this._environment._currentEnvironment, myScope);
      this._environment._currentEnvironment.addons = addons;
    }

    return result;
  }),

  



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
      let experiments = scope.Experiments.instance();
      let activeExperiment = experiments.getActiveExperimentID();
      if (activeExperiment) {
        experimentInfo.id = activeExperiment;
        experimentInfo.branch = experiments.getActiveExperimentBranch();
      }
    } catch(e) {
      
    }

    return experimentInfo;
  },
};

function EnvironmentCache() {
  this._log = Log.repository.getLoggerWithMessagePrefix(
    LOGGER_NAME, "TelemetryEnvironment::");
  this._log.trace("constructor");

  this._shutdown = false;

  
  this._changeListeners = new Map();

  
  this._lastEnvironmentChangeDate = null;

  
  
  this._watchedPrefs = DEFAULT_ENVIRONMENT_PREFS;

  this._currentEnvironment = {
    build: this._getBuild(),
    partner: this._getPartner(),
    system: this._getSystem(),
  };

  this._updateSettings();

#ifndef MOZ_WIDGET_ANDROID
  this._currentEnvironment.profile = {};
#endif

  
  

  this._addonBuilder = new EnvironmentAddonBuilder(this);

  this._initTask = Promise.all([this._addonBuilder.init(), this._updateProfile()])
    .then(
      () => {
        this._initTask = null;
        this._startWatchingPrefs();
        this._addonBuilder.watchForChanges();
        return this.currentEnvironment;
      },
      (err) => {
        
        this._log.error("error while initializing", err);
        this._initTask = null;
        this._startWatchingPrefs();
        this._addonBuilder.watchForChanges();
        return this.currentEnvironment;
      });
}
EnvironmentCache.prototype = {
  




  get currentEnvironment() {
    return Cu.cloneInto(this._currentEnvironment, myScope);
  },

  



  onInitialized: function() {
    if (this._initTask) {
      return this._initTask;
    }
    return Promise.resolve(this.currentEnvironment);
  },

  






  registerChangeListener: function (name, listener) {
    this._log.trace("registerChangeListener for " + name);
    if (this._shutdown) {
      this._log.warn("registerChangeListener - already shutdown");
      return;
    }
    this._changeListeners.set(name, listener);
  },

  




  unregisterChangeListener: function (name) {
    this._log.trace("unregisterChangeListener for " + name);
    if (this._shutdown) {
      this._log.warn("registerChangeListener - already shutdown");
      return;
    }
    this._changeListeners.delete(name);
  },

  



  _watchPreferences: function (aPreferences) {
    this._stopWatchingPrefs();
    this._watchedPrefs = aPreferences;
    this._updateSettings();
    this._startWatchingPrefs();
  },

  





  _getPrefData: function () {
    let prefData = {};
    for (let pref in this._watchedPrefs) {
      
      if (!Preferences.isSet(pref) ||
          this._watchedPrefs[pref] == TelemetryEnvironment.RECORD_PREF_NOTIFY_ONLY) {
        continue;
      }

      
      
      let prefValue = undefined;
      if (this._watchedPrefs[pref] == TelemetryEnvironment.RECORD_PREF_STATE) {
        prefValue = "<user-set>";
      } else {
        prefValue = Preferences.get(pref, null);
      }
      prefData[pref] = prefValue;
    }
    return prefData;
  },

  


  _startWatchingPrefs: function () {
    this._log.trace("_startWatchingPrefs - " + this._watchedPrefs);

    for (let pref in this._watchedPrefs) {
      Preferences.observe(pref, this._onPrefChanged, this);
    }
  },

  _onPrefChanged: function() {
    this._log.trace("_onPrefChanged");
    let oldEnvironment = Cu.cloneInto(this._currentEnvironment, myScope);
    this._updateSettings();
    this._onEnvironmentChange("pref-changed", oldEnvironment);
  },

  


  _stopWatchingPrefs: function () {
    this._log.trace("_stopWatchingPrefs");

    for (let pref in this._watchedPrefs) {
      Preferences.ignore(pref, this._onPrefChanged, this);
    }
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

  


  _updateSettings: function () {
    let updateChannel = null;
    try {
      updateChannel = UpdateChannel.get();
    } catch (e) {}

    this._currentEnvironment.settings = {
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

  



  _updateProfile: Task.async(function* () {
    let profileAccessor = new ProfileAge(null, this._log);

    let creationDate = yield profileAccessor.created;
    let resetDate = yield profileAccessor.reset;

    this._currentEnvironment.profile.creationDate =
      truncateToDays(creationDate);
    if (resetDate) {
      this._currentEnvironment.profile.resetDate = truncateToDays(resetDate);
    }
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
    partnerData.partnerNames = partnerBranch.getChildList("");

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

  _onEnvironmentChange: function (what, oldEnvironment) {
    this._log.trace("_onEnvironmentChange for " + what);
    if (this._shutdown) {
      this._log.trace("_onEnvironmentChange - Already shut down.");
      return;
    }

    
    let now = Policy.now();
    if (this._lastEnvironmentChangeDate &&
        (CHANGE_THROTTLE_INTERVAL_MS >=
         (now.getTime() - this._lastEnvironmentChangeDate.getTime()))) {
      this._log.trace("_onEnvironmentChange - throttling changes, now: " + now +
                      ", last change: " + this._lastEnvironmentChangeDate);
      return;
    }

    this._lastEnvironmentChangeDate = now;

    for (let [name, listener] of this._changeListeners) {
      try {
        this._log.debug("_onEnvironmentChange - calling " + name);
        listener(what, oldEnvironment);
      } catch (e) {
        this._log.error("_onEnvironmentChange - listener " + name + " caught error", e);
      }
    }
  },
};
