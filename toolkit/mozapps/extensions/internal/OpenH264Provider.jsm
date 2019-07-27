



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

const URI_EXTENSION_STRINGS    = "chrome://mozapps/locale/extensions/extensions.properties";
const STRING_TYPE_NAME         = "type.%ID%.name";

const OPENH264_PLUGIN_ID       = "openh264-plugin@cisco.com";
const OPENH264_PREF_BRANCH     = "media.openh264.";
const OPENH264_PREF_ENABLED    = "enabled";
const OPENH264_PREF_PATH       = "path";
const OPENH264_PREF_VERSION    = "version";
const OPENH264_PREF_LASTUPDATE = "lastUpdate";
const OPENH264_PREF_AUTOUPDATE = "autoupdate";
const OPENH264_PREF_PROVIDERENABLED = "providerEnabled";
const OPENH264_HOMEPAGE_URL    = "http://www.openh264.org/";
const OPENH264_OPTIONS_URL     = "chrome://mozapps/content/extensions/openH264Prefs.xul";

XPCOMUtils.defineLazyGetter(this, "pluginsBundle",
  () => Services.strings.createBundle("chrome://global/locale/plugins.properties"));
XPCOMUtils.defineLazyGetter(this, "prefs",
  () => new Preferences(OPENH264_PREF_BRANCH));
XPCOMUtils.defineLazyGetter(this, "gmpService",
  () => Cc["@mozilla.org/gecko-media-plugin-service;1"].getService(Ci.mozIGeckoMediaPluginService));





let OpenH264Wrapper = Object.freeze({
  optionsType: AddonManager.OPTIONS_TYPE_INLINE,
  optionsURL: OPENH264_OPTIONS_URL,

  get id() { return OPENH264_PLUGIN_ID; },
  get type() { return "plugin"; },
  get name() { return pluginsBundle.GetStringFromName("openH264_name"); },
  get creator() { return null; },
  get homepageURL() { return OPENH264_HOMEPAGE_URL; },

  get description() { return pluginsBundle.GetStringFromName("openH264_description"); },

  get version() {
    if (this.isInstalled) {
      return prefs.get(OPENH264_PREF_VERSION, "");
    }
    return "";
  },

  get isActive() { return !this.userDisabled; },
  get appDisabled() { return false; },

  get userDisabled() { return !prefs.get(OPENH264_PREF_ENABLED, false); },
  set userDisabled(aVal) { prefs.set(OPENH264_PREF_ENABLED, aVal === false); },

  get blocklistState() { return Ci.nsIBlocklistService.STATE_NOT_BLOCKED; },
  get size() { return 0; },
  get scope() { return AddonManager.SCOPE_APPLICATION; },
  get pendingOperations() { return AddonManager.PENDING_NONE; },

  get operationsRequiringRestart() { return AddonManager.OP_NEEDS_RESTART_NONE },

  get permissions() {
    let permissions = AddonManager.PERM_CAN_UPGRADE;
    if (!this.appDisabled) {
      permissions |= this.userDisabled ? AddonManager.PERM_CAN_ENABLE :
                                         AddonManager.PERM_CAN_DISABLE;
    }
    return permissions;
  },

  get updateDate() {
    let time = Number(prefs.get(OPENH264_PREF_LASTUPDATE, null));
    if (time !== NaN && this.isInstalled) {
      return new Date(time)
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
    if (!prefs.isSet(OPENH264_PREF_AUTOUPDATE)) {
      return AddonManager.AUTOUPDATE_DEFAULT;
    }

    return prefs.get(OPENH264_PREF_AUTOUPDATE, true) ?
      AddonManager.AUTOUPDATE_ENABLE : AddonManager.AUTOUPDATE_DISABLE;
  },

  set applyBackgroundUpdates(aVal) {
    if (aVal == AddonManager.AUTOUPDATE_DEFAULT) {
      prefs.reset(OPENH264_PREF_AUTOUPDATE);
    } else if (aVal == AddonManager.AUTOUPDATE_ENABLE) {
      prefs.set(OPENH264_PREF_AUTOUPDATE, true);
    } else if (aVal == AddonManager.AUTOUPDATE_DISABLE) {
      prefs.set(OPENH264_PREF_AUTOUPDATE, false);
    }
  },

  findUpdates: function(aListener, aReason, aAppVersion, aPlatformVersion) {
    

    if ("onNoCompatibilityUpdateAvailable" in aListener)
      aListener.onNoCompatibilityUpdateAvailable(this);
    if ("onNoUpdateAvailable" in aListener)
      aListener.onNoUpdateAvailable(this);
    if ("onUpdateFinished" in aListener)
      aListener.onUpdateFinished(this);
  },

  get pluginMimeTypes() { return []; },
  get pluginLibraries() {
    let path = prefs.get(OPENH264_PREF_PATH, null);
    return path && path.length ? [OS.Path.basename(path)] : [];
  },
  get pluginFullpath() {
    let path = prefs.get(OPENH264_PREF_PATH, null);
    return path && path.length ? [path] : [];
  },

  get isInstalled() {
    let path = prefs.get(OPENH264_PREF_PATH, "");
    return path.length > 0;
  },
});

let OpenH264Provider = {
  startup: function() {
    Services.obs.addObserver(this, AddonManager.OPTIONS_NOTIFICATION_DISPLAYED, false);
    prefs.observe(OPENH264_PREF_ENABLED, this.onPrefEnabledChanged, this);
    prefs.observe(OPENH264_PREF_PATH, this.onPrefPathChanged, this);
  },

  shutdown: function() {
    Services.obs.removeObserver(this, AddonManager.OPTIONS_NOTIFICATION_DISPLAYED);
    prefs.ignore(OPENH264_PREF_ENABLED, this.onPrefEnabledChanged, this);
    prefs.ignore(OPENH264_PREF_PATH, this.onPrefPathChanged, this);
  },

  onPrefEnabledChanged: function() {
    let wrapper = OpenH264Wrapper;

    AddonManagerPrivate.callAddonListeners(wrapper.isActive ?
                                           "onEnabling" : "onDisabling",
                                           wrapper, false);
    AddonManagerPrivate.callAddonListeners(wrapper.isActive ?
                                           "onEnabled" : "onDisabled",
                                           wrapper);
  },

  onPrefPathChanged: function() {
    let wrapper = OpenH264Wrapper;

    AddonManagerPrivate.callAddonListeners("onUninstalling", wrapper, false);
    if (this.gmpPath) {
      gmpService.removePluginDirectory(this.gmpPath);
    }
    AddonManagerPrivate.callAddonListeners("onUninstalled", wrapper);

    AddonManagerPrivate.callInstallListeners("onExternalInstall", null, wrapper, null, false);
    this.gmpPath = prefs.get(OPENH264_PREF_PATH, null);
    if (this.gmpPath) {
      gmpService.addPluginDirectory(this.gmpPath);
    }
    AddonManagerPrivate.callAddonListeners("onInstalled", wrapper);
  },

  buildWrapper: function() {
    let description = pluginsBundle.GetStringFromName("openH264_description");
    return new OpenH264Wrapper(OPENH264_PLUGIN_ID,
                               OPENH264_PLUGIN_NAME,
                               description);
  },

  getAddonByID: function(aId, aCallback) {
    if (this.isEnabled && aId == OPENH264_PLUGIN_ID) {
      aCallback(OpenH264Wrapper);
    } else {
      aCallback(null);
    }
  },

  getAddonsByTypes: function(aTypes, aCallback) {
    if (!this.isEnabled || aTypes && aTypes.indexOf("plugin") < 0) {
      aCallback([]);
      return;
    }

    aCallback([OpenH264Wrapper]);
  },

  get isEnabled() {
    return prefs.get(OPENH264_PREF_PROVIDERENABLED, false);
  },
};

AddonManagerPrivate.registerProvider(OpenH264Provider, [
  new AddonManagerPrivate.AddonType("plugin", URI_EXTENSION_STRINGS,
                                    STRING_TYPE_NAME,
                                    AddonManager.VIEW_TYPE_LIST, 6000,
                                    AddonManager.TYPE_SUPPORTS_ASK_TO_ACTIVATE)
]);
