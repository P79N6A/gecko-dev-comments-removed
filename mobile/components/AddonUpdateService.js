



































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gObs",
                                   "@mozilla.org/observer-service;1",
                                   "nsIObserverService");

XPCOMUtils.defineLazyGetter(this, "AddonManager", function() {
  Components.utils.import("resource://gre/modules/AddonManager.jsm");
  return AddonManager;
});

XPCOMUtils.defineLazyServiceGetter(this, "gIO",
                                   "@mozilla.org/network/io-service;1",
                                   "nsIIOService");

XPCOMUtils.defineLazyServiceGetter(this, "gPref",
                                   "@mozilla.org/preferences-service;1",
                                   "nsIPrefBranch2");

function getPref(func, preference, defaultValue) {
  try {
    return gPref[func](preference);
  }
  catch (e) {}
  return defaultValue;
}





const PREF_ADDON_UPDATE_ENABLED  = "extensions.autoupdate.enabled";

var gNeedsRestart = false;

function AddonUpdateService() {}

AddonUpdateService.prototype = {
  classDescription: "Add-on auto-update management",
  classID: Components.ID("{93c8824c-9b87-45ae-bc90-5b82a1e4d877}"),
  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITimerCallback]),

  notify: function aus_notify(aTimer) {
    if (aTimer && !getPref("getBoolPref", PREF_ADDON_UPDATE_ENABLED, true))
      return;

    
    if (gNeedsRestart)
      return;

    gIO.offline = false;

    AddonManager.getAddonsByTypes(null, function(aAddonList) {
      aAddonList.forEach(function(aAddon) {
        if (aAddon.permissions & AddonManager.PERM_CAN_UPGRADE) {
          let data = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
          data.data = JSON.stringify({ id: aAddon.id, name: aAddon.name });
          gObs.notifyObservers(data, "addon-update-started", null);

          let listener = new UpdateCheckListener();
          aAddon.findUpdates(listener, AddonManager.UPDATE_WHEN_USER_REQUESTED);
        }
      });
    });
  }
};






function UpdateCheckListener() {
  this._status = null;
  this._version = null;
}

UpdateCheckListener.prototype = {
  onCompatibilityUpdateAvailable: function(aAddon) {
    this._status = "compatibility";
  },

  onUpdateAvailable: function(aAddon, aInstall) {
    this._status = "update";
    this._version = aInstall.version;
    aInstall.install();
  },

  onNoUpdateAvailable: function(aAddon) {
    if (!this._status)
      this._status = "no-update";
  },

  onUpdateFinished: function(aAddon, aError) {
    let data = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
    if (this._version)
      data.data = JSON.stringify({ id: aAddon.id, name: aAddon.name, version: this._version });
    else
      data.data = JSON.stringify({ id: aAddon.id, name: aAddon.name });

    if (aError)
      this._status = "error";

    gObs.notifyObservers(data, "addon-update-ended", this._status);
  }
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([AddonUpdateService]);

