



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

this.EXPORTED_SYMBOLS = [];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/AddonManager.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/GMPUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(
  this, "GMPInstallManager", "resource://gre/modules/GMPInstallManager.jsm");
XPCOMUtils.defineLazyModuleGetter(
  this, "setTimeout", "resource://gre/modules/Timer.jsm");

const URI_EXTENSION_STRINGS  = "chrome://mozapps/locale/extensions/extensions.properties";
const STRING_TYPE_NAME       = "type.%ID%.name";

const SEC_IN_A_DAY           = 24 * 60 * 60;

const GMP_CHECK_DELAY        = 10 * 1000; 

const NS_GRE_DIR             = "GreD";
const CLEARKEY_PLUGIN_ID     = "gmp-clearkey";
const CLEARKEY_VERSION       = "0.1";

const GMP_LICENSE_INFO       = "gmp_license_info";
const GMP_LEARN_MORE         = "learn_more_label";

const GMP_PLUGINS = [
  {
    id:              OPEN_H264_ID,
    name:            "openH264_name",
    description:     "openH264_description2",
    
    
    
    licenseURL:      "chrome://mozapps/content/extensions/OpenH264-license.txt",
    homepageURL:     "http://www.openh264.org/",
    optionsURL:      "chrome://mozapps/content/extensions/gmpPrefs.xul"
  },
  {
    id:              EME_ADOBE_ID,
    name:            "eme-adobe_name",
    description:     "eme-adobe_description",
    
    
    get learnMoreURL() {
      return Services.urlFormatter.formatURLPref("app.support.baseURL") + "drm-content";
    },
    licenseURL:      "http://help.adobe.com/en_US/primetime/drm/HTML5_CDM_EULA/index.html",
    homepageURL:     "http://help.adobe.com/en_US/primetime/drm/index.html",
    optionsURL:      "chrome://mozapps/content/extensions/gmpPrefs.xul",
    isEME:           true
  }];

XPCOMUtils.defineLazyGetter(this, "pluginsBundle",
  () => Services.strings.createBundle("chrome://global/locale/plugins.properties"));
XPCOMUtils.defineLazyGetter(this, "gmpService",
  () => Cc["@mozilla.org/gecko-media-plugin-service;1"].getService(Ci.mozIGeckoMediaPluginService));

let gLogger;
let gLogAppenderDump = null;

function configureLogging() {
  if (!gLogger) {
    gLogger = Log.repository.getLogger("Toolkit.GMP");
    gLogger.addAppender(new Log.ConsoleAppender(new Log.BasicFormatter()));
  }
  gLogger.level = GMPPrefs.get(GMPPrefs.KEY_LOGGING_LEVEL, Log.Level.Warn);

  let logDumping = GMPPrefs.get(GMPPrefs.KEY_LOGGING_DUMP, false);
  if (logDumping != !!gLogAppenderDump) {
    if (logDumping) {
      gLogAppenderDump = new Log.DumpAppender(new Log.BasicFormatter());
      gLogger.addAppender(gLogAppenderDump);
    } else {
      gLogger.removeAppender(gLogAppenderDump);
      gLogAppenderDump = null;
    }
  }
}







function GMPWrapper(aPluginInfo) {
  this._plugin = aPluginInfo;
  this._log =
    Log.repository.getLoggerWithMessagePrefix("Toolkit.GMP",
                                              "GMPWrapper(" +
                                              this._plugin.id + ") ");
  Preferences.observe(GMPPrefs.getPrefKey(GMPPrefs.KEY_PLUGIN_ENABLED,
                                          this._plugin.id),
                      this.onPrefEnabledChanged, this);
  Preferences.observe(GMPPrefs.getPrefKey(GMPPrefs.KEY_PLUGIN_VERSION,
                                          this._plugin.id),
                      this.onPrefVersionChanged, this);
  if (this._plugin.isEME) {
    Preferences.observe(GMPPrefs.KEY_EME_ENABLED,
                        this.onPrefEMEGlobalEnabledChanged, this);
  }
}

GMPWrapper.prototype = {
  
  _updateTask: null,
  _gmpPath: null,
  _isUpdateCheckPending: false,

  optionsType: AddonManager.OPTIONS_TYPE_INLINE,
  get optionsURL() { return this._plugin.optionsURL; },

  set gmpPath(aPath) { this._gmpPath = aPath; },
  get gmpPath() {
    if (!this._gmpPath && this.isInstalled) {
      this._gmpPath = OS.Path.join(OS.Constants.Path.profileDir,
                                   this._plugin.id,
                                   GMPPrefs.get(GMPPrefs.KEY_PLUGIN_VERSION,
                                                null, this._plugin.id));
    }
    return this._gmpPath;
  },

  get id() { return this._plugin.id; },
  get type() { return "plugin"; },
  get isGMPlugin() { return true; },
  get name() { return this._plugin.name; },
  get creator() { return null; },
  get homepageURL() { return this._plugin.homepageURL; },

  get description() { return this._plugin.description; },
  get fullDescription() { return this._plugin.fullDescription; },

  get version() { return GMPPrefs.get(GMPPrefs.KEY_PLUGIN_VERSION, null,
                                      this._plugin.id); },

  get isActive() { return !this.appDisabled && !this.userDisabled; },
  get appDisabled() {
    if (this._plugin.isEME && !GMPPrefs.get(GMPPrefs.KEY_EME_ENABLED, true)) {
      
      return true;
    }
   return false;
  },

  get userDisabled() {
    return !GMPPrefs.get(GMPPrefs.KEY_PLUGIN_ENABLED, true, this._plugin.id);
  },
  set userDisabled(aVal) { GMPPrefs.set(GMPPrefs.KEY_PLUGIN_ENABLED,
                                        aVal === false,
                                        this._plugin.id); },

  get blocklistState() { return Ci.nsIBlocklistService.STATE_NOT_BLOCKED; },
  get size() { return 0; },
  get scope() { return AddonManager.SCOPE_APPLICATION; },
  get pendingOperations() { return AddonManager.PENDING_NONE; },

  get operationsRequiringRestart() { return AddonManager.OP_NEEDS_RESTART_NONE },

  get permissions() {
    let permissions = 0;
    if (!this.appDisabled) {
      permissions |= AddonManager.PERM_CAN_UPGRADE;
      permissions |= this.userDisabled ? AddonManager.PERM_CAN_ENABLE :
                                         AddonManager.PERM_CAN_DISABLE;
    }
    return permissions;
  },

  get updateDate() {
    let time = Number(GMPPrefs.get(GMPPrefs.KEY_PLUGIN_LAST_UPDATE, null,
                                   this._plugin.id));
    if (time !== NaN && this.isInstalled) {
      return new Date(time * 1000)
    }
    return null;
  },

  get isCompatible() {
    return true;
  },

  get isPlatformCompatible() {
    return true;
  },

  get providesUpdatesSecurely() {
    return true;
  },

  get foreignInstall() {
    return false;
  },

  isCompatibleWith: function(aAppVersion, aPlatformVersion) {
    return true;
  },

  get applyBackgroundUpdates() {
    if (!GMPPrefs.isSet(GMPPrefs.KEY_PLUGIN_AUTOUPDATE, this._plugin.id)) {
      return AddonManager.AUTOUPDATE_DEFAULT;
    }

    return GMPPrefs.get(GMPPrefs.KEY_PLUGIN_AUTOUPDATE, true, this._plugin.id) ?
      AddonManager.AUTOUPDATE_ENABLE : AddonManager.AUTOUPDATE_DISABLE;
  },

  set applyBackgroundUpdates(aVal) {
    if (aVal == AddonManager.AUTOUPDATE_DEFAULT) {
      GMPPrefs.reset(GMPPrefs.KEY_PLUGIN_AUTOUPDATE, this._plugin.id);
    } else if (aVal == AddonManager.AUTOUPDATE_ENABLE) {
      GMPPrefs.set(GMPPrefs.KEY_PLUGIN_AUTOUPDATE, true, this._plugin.id);
    } else if (aVal == AddonManager.AUTOUPDATE_DISABLE) {
      GMPPrefs.set(GMPPrefs.KEY_PLUGIN_AUTOUPDATE, false, this._plugin.id);
    }
  },

  findUpdates: function(aListener, aReason, aAppVersion, aPlatformVersion) {
    this._log.trace("findUpdates() - " + this._plugin.id + " - reason=" +
                    aReason);

    AddonManagerPrivate.callNoUpdateListeners(this, aListener);

    if (aReason === AddonManager.UPDATE_WHEN_PERIODIC_UPDATE) {
      if (!AddonManager.shouldAutoUpdate(this)) {
        this._log.trace("findUpdates() - " + this._plugin.id +
                        " - no autoupdate");
        return Promise.resolve(false);
      }

      let secSinceLastCheck =
        Date.now() / 1000 - Preferences.get(GMPPrefs.KEY_UPDATE_LAST_CHECK, 0);
      if (secSinceLastCheck <= SEC_IN_A_DAY) {
        this._log.trace("findUpdates() - " + this._plugin.id +
                        " - last check was less then a day ago");
        return Promise.resolve(false);
      }
    } else if (aReason !== AddonManager.UPDATE_WHEN_USER_REQUESTED) {
      this._log.trace("findUpdates() - " + this._plugin.id +
                      " - the given reason to update is not supported");
      return Promise.resolve(false);
    }

    if (this._updateTask !== null) {
      this._log.trace("findUpdates() - " + this._plugin.id +
                      " - update task already running");
      return this._updateTask;
    }

    this._updateTask = Task.spawn(function* GMPProvider_updateTask() {
      this._log.trace("findUpdates() - updateTask");
      try {
        let installManager = new GMPInstallManager();
        let gmpAddons = yield installManager.checkForAddons();
        let update = gmpAddons.find(function(aAddon) {
          return aAddon.id === this._plugin.id;
        }, this);
        if (update && update.isValid && !update.isInstalled) {
          this._log.trace("findUpdates() - found update for " +
                          this._plugin.id + ", installing");
          yield installManager.installAddon(update);
        } else {
          this._log.trace("findUpdates() - no updates for " + this._plugin.id);
        }
        this._log.info("findUpdates() - updateTask succeeded for " +
                       this._plugin.id);
      } catch (e) {
        this._log.error("findUpdates() - updateTask for " + this._plugin.id +
                        " threw", e);
        throw e;
      } finally {
        this._updateTask = null;
        return true;
      }
    }.bind(this));

    return this._updateTask;
  },

  get pluginMimeTypes() { return []; },
  get pluginLibraries() {
    if (this.isInstalled) {
      let path = this.version;
      return [path];
    }
    return [];
  },
  get pluginFullpath() {
    if (this.isInstalled) {
      let path = OS.Path.join(OS.Constants.Path.profileDir,
                              this._plugin.id,
                              this.version);
      return [path];
    }
    return [];
  },

  get isInstalled() {
    return this.version && this.version.length > 0;
  },

  _handleEnabledChanged: function() {
    AddonManagerPrivate.callAddonListeners(this.isActive ?
                                           "onEnabling" : "onDisabling",
                                           this, false);
    if (this._gmpPath) {
      if (this.isActive) {
        this._log.info("onPrefEnabledChanged() - adding gmp directory " +
                       this._gmpPath);
        gmpService.addPluginDirectory(this._gmpPath);
      } else {
        this._log.info("onPrefEnabledChanged() - removing gmp directory " +
                       this._gmpPath);
        gmpService.removePluginDirectory(this._gmpPath);
      }
    }
    AddonManagerPrivate.callAddonListeners(this.isActive ?
                                           "onEnabled" : "onDisabled",
                                           this);
  },

  onPrefEMEGlobalEnabledChanged: function() {
    AddonManagerPrivate.callAddonListeners("onPropertyChanged", this,
                                           ["appDisabled"]);
    if (this.appDisabled) {
      this.uninstallPlugin();
    } else {
      AddonManagerPrivate.callInstallListeners("onExternalInstall", null, this,
                                               null, false);
      AddonManagerPrivate.callAddonListeners("onInstalling", this, false);
      AddonManagerPrivate.callAddonListeners("onInstalled", this);
      if (!this._isUpdateCheckPending) {
        this._isUpdateCheckPending = true;
        GMPPrefs.reset(GMPPrefs.KEY_UPDATE_LAST_CHECK, null);
        
        
        setTimeout(() => {
          if (!this.appDisabled) {
            let gmpInstallManager = new GMPInstallManager();
            
            
            gmpInstallManager.simpleCheckAndInstall().then(null, () => {});
          }
          this._isUpdateCheckPending = false;
        }, GMP_CHECK_DELAY);
      }
    }
    if (!this.userDisabled) {
      this._handleEnabledChanged();
    }
  },

  onPrefEnabledChanged: function() {
    if (!this._plugin.isEME || !this.appDisabled) {
      this._handleEnabledChanged();
    }
  },

  onPrefVersionChanged: function() {
    AddonManagerPrivate.callAddonListeners("onUninstalling", this, false);
    if (this._gmpPath) {
      this._log.info("onPrefVersionChanged() - unregistering gmp directory " +
                     this._gmpPath);
      gmpService.removeAndDeletePluginDirectory(this._gmpPath);
    }
    AddonManagerPrivate.callAddonListeners("onUninstalled", this);

    AddonManagerPrivate.callInstallListeners("onExternalInstall", null, this,
                                             null, false);
    AddonManagerPrivate.callAddonListeners("onInstalling", this, false);
    this._gmpPath = null;
    if (this.isInstalled) {
      this._gmpPath = OS.Path.join(OS.Constants.Path.profileDir,
                                   this._plugin.id,
                                   GMPPrefs.get(GMPPrefs.KEY_PLUGIN_VERSION,
                                                null, this._plugin.id));
    }
    if (this._gmpPath && this.isActive) {
      this._log.info("onPrefVersionChanged() - registering gmp directory " +
                     this._gmpPath);
      gmpService.addPluginDirectory(this._gmpPath);
    }
    AddonManagerPrivate.callAddonListeners("onInstalled", this);
  },

  uninstallPlugin: function() {
    AddonManagerPrivate.callAddonListeners("onUninstalling", this, false);
    if (this.gmpPath) {
      this._log.info("uninstallPlugin() - unregistering gmp directory " +
                     this.gmpPath);
      gmpService.removeAndDeletePluginDirectory(this.gmpPath);
    }
    GMPPrefs.reset(GMPPrefs.KEY_PLUGIN_VERSION, this.id);
    AddonManagerPrivate.callAddonListeners("onUninstalled", this);
  },

  shutdown: function() {
    Preferences.ignore(GMPPrefs.getPrefKey(GMPPrefs.KEY_PLUGIN_ENABLED,
                                           this._plugin.id),
                       this.onPrefEnabledChanged, this);
    Preferences.ignore(GMPPrefs.getPrefKey(GMPPrefs.KEY_PLUGIN_VERSION,
                                           this._plugin.id),
                       this.onPrefVersionChanged, this);
    if (this._plugin.isEME) {
      Preferences.ignore(GMPPrefs.KEY_EME_ENABLED,
                         this.onPrefEMEGlobalEnabledChanged, this);
    }
    return this._updateTask;
  },
};

let GMPProvider = {
  get name() { return "GMPProvider"; },

  _plugins: null,

  startup: function() {
    configureLogging();
    this._log = Log.repository.getLoggerWithMessagePrefix("Toolkit.GMP",
                                                          "GMPProvider.");
    let telemetry = {};
    this.buildPluginList();
    this.ensureProperCDMInstallState();

    Preferences.observe(GMPPrefs.KEY_LOG_BASE, configureLogging);

    for (let [id, plugin] of this._plugins) {
      let wrapper = plugin.wrapper;
      let gmpPath = wrapper.gmpPath;
      let isEnabled = wrapper.isActive;
      this._log.trace("startup - enabled=" + isEnabled + ", gmpPath=" +
                      gmpPath);

      if (gmpPath && isEnabled) {
        this._log.info("startup - adding gmp directory " + gmpPath);
        try {
          gmpService.addPluginDirectory(gmpPath);
        } catch (e if e.name == 'NS_ERROR_NOT_AVAILABLE') {
          this._log.warn("startup - adding gmp directory failed with " +
                         e.name + " - sandboxing not available?", e);
        }
      }

      if (this.isEnabled) {
        telemetry[id] = {
          userDisabled: wrapper.userDisabled,
          version: wrapper.version,
          applyBackgroundUpdates: wrapper.applyBackgroundUpdates,
        };
      }
    }

    if (Preferences.get(GMPPrefs.KEY_EME_ENABLED, false)) {
      try {
        let greDir = Services.dirsvc.get(NS_GRE_DIR,
                                         Ci.nsILocalFile);
        let clearkeyPath = OS.Path.join(greDir.path,
                                        CLEARKEY_PLUGIN_ID,
                                        CLEARKEY_VERSION);
        this._log.info("startup - adding clearkey CDM directory " +
                       clearkeyPath);
        gmpService.addPluginDirectory(clearkeyPath);
      } catch (e) {
        this._log.warn("startup - adding clearkey CDM failed", e);
      }
    }

    AddonManagerPrivate.setTelemetryDetails("GMP", telemetry);
  },

  shutdown: function() {
    this._log.trace("shutdown");
    Preferences.ignore(GMPPrefs.KEY_LOG_BASE, configureLogging);

    let shutdownTask = Task.spawn(function* GMPProvider_shutdownTask() {
      this._log.trace("shutdown - shutdownTask");
      let shutdownSucceeded = true;

      for (let plugin of this._plugins.values()) {
        try {
          yield plugin.wrapper.shutdown();
        } catch (e) {
          shutdownSucceeded = false;
        }
      }

      this._plugins = null;

      if (!shutdownSucceeded) {
        throw new Error("Shutdown failed");
      }
    }.bind(this));

    return shutdownTask;
  },

  getAddonByID: function(aId, aCallback) {
    if (!this.isEnabled) {
      aCallback(null);
      return;
    }

    let plugin = this._plugins.get(aId);
    if (plugin && !GMPUtils.isPluginHidden(plugin)) {
      aCallback(plugin.wrapper);
    } else {
      aCallback(null);
    }
  },

  getAddonsByTypes: function(aTypes, aCallback) {
    if (!this.isEnabled ||
        (aTypes && aTypes.indexOf("plugin") < 0)) {
      aCallback([]);
      return;
    }

    let results = [p.wrapper for ([id, p] of this._plugins)
                    if (!GMPUtils.isPluginHidden(p))];
    aCallback(results);
  },

  get isEnabled() {
    return GMPPrefs.get(GMPPrefs.KEY_PROVIDER_ENABLED, false);
  },

  generateFullDescription: function(aPlugin) {
    let rv = [];
    for (let [urlProp, labelId] of [["learnMoreURL", GMP_LEARN_MORE],
                                    ["licenseURL", GMP_LICENSE_INFO]]) {
      if (aPlugin[urlProp]) {
        let label = pluginsBundle.GetStringFromName(labelId);
        rv.push(`<xhtml:a href="${aPlugin[urlProp]}" target="_blank">${label}</xhtml:a>.`);
      }
    }
    return rv.length ? rv.join("<xhtml:br /><xhtml:br />") : undefined;
  },

  buildPluginList: function() {
    this._plugins = new Map();
    for (let aPlugin of GMP_PLUGINS) {
      let plugin = {
        id: aPlugin.id,
        name: pluginsBundle.GetStringFromName(aPlugin.name),
        description: pluginsBundle.GetStringFromName(aPlugin.description),
        homepageURL: aPlugin.homepageURL,
        optionsURL: aPlugin.optionsURL,
        wrapper: null,
        isEME: aPlugin.isEME,
      };
      plugin.fullDescription = this.generateFullDescription(aPlugin);
      plugin.wrapper = new GMPWrapper(plugin);
      this._plugins.set(plugin.id, plugin);
    }
  },

  ensureProperCDMInstallState: function() {
    if (!GMPPrefs.get(GMPPrefs.KEY_EME_ENABLED, true)) {
      for (let [id, plugin] of this._plugins) {
        if (plugin.isEME && plugin.wrapper.isInstalled) {
          gmpService.addPluginDirectory(plugin.wrapper.gmpPath);
          plugin.wrapper.uninstallPlugin();
        }
      }
    }
  },
};

AddonManagerPrivate.registerProvider(GMPProvider, [
  new AddonManagerPrivate.AddonType("plugin", URI_EXTENSION_STRINGS,
                                    STRING_TYPE_NAME,
                                    AddonManager.VIEW_TYPE_LIST, 6000,
                                    AddonManager.TYPE_SUPPORTS_ASK_TO_ACTIVATE)
]);
