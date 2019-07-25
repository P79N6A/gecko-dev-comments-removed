



































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "gObs",
                                   "@mozilla.org/observer-service;1",
                                   "nsIObserverService");

XPCOMUtils.defineLazyServiceGetter(this, "gExtMgr",
                                   "@mozilla.org/extensions/manager;1",
                                   "nsIExtensionManager");

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
const PREF_ADDON_UPDATE_INTERVAL = "extensions.autoupdate.interval";

var gNeedsRestart = false;

function AddonUpdateService() {}

AddonUpdateService.prototype = {
  classDescription: "Add-on auto-update management",
  contractID: "@mozilla.org/browser/addon-update-service;1",
  classID: Components.ID("{93c8824c-9b87-45ae-bc90-5b82a1e4d877}"),
  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsITimerCallback]),

  _xpcom_categories: [{ category: "update-timer",
                        value: "@mozilla.org/browser/addon-update-service;1," +
                               "getService,auto-addon-background-update-timer," +
                                PREF_ADDON_UPDATE_INTERVAL + ",86400" }],

  notify: function aus_notify(aTimer) {
    if (aTimer && !getPref("getBoolPref", PREF_ADDON_UPDATE_ENABLED, true))
      return;

    
    if (gNeedsRestart)
      return;

    gIO.offline = false;

    
    let items = [];
    let listener = new UpdateCheckListener();
    gExtMgr.update(items, items.length, Ci.nsIExtensionManager.UPDATE_CHECK_NEWVERSION, listener);
  }
};






function UpdateCheckListener() {
  this._addons = [];
}

UpdateCheckListener.prototype = {
  
  
  onUpdateStarted: function ucl_onUpdateStarted() {
  },

  onUpdateEnded: function ucl_onUpdateEnded() {
    if (!this._addons.length)
      return;

    
    let items = [];
    for (let i = 0; i < this._addons.length; i++)
      items.push(gExtMgr.getItemForID(this._addons[i]));

    
    gExtMgr.addDownloads(items, items.length, null);

    
    gNeedsRestart = true;
  },

  onAddonUpdateStarted: function ucl_onAddonUpdateStarted(aAddon) {
    gObs.notifyObservers(aAddon, "addon-update-started", null);
  },

  onAddonUpdateEnded: function ucl_onAddonUpdateEnded(aAddon, aStatus) {
    gObs.notifyObservers(aAddon, "addon-update-ended", aStatus);
    
    
    if (aStatus == Ci.nsIAddonUpdateCheckListener.STATUS_UPDATE)
      this._addons.push(aAddon.id);
  },

  QueryInterface: function ucl_QueryInterface(aIID) {
    if (!aIID.equals(Ci.nsIAddonUpdateCheckListener) &&
        !aIID.equals(Ci.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};

function NSGetModule(aCompMgr, aFileSpec) {
  return XPCOMUtils.generateModule([AddonUpdateService]);
}

