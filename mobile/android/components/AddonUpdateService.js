



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AddonManager",
                                  "resource://gre/modules/AddonManager.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "AddonRepository",
                                  "resource://gre/modules/AddonRepository.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "OS", "resource://gre/modules/osfile.jsm");

function getPref(func, preference, defaultValue) {
  try {
    return Services.prefs[func](preference);
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

    Services.io.offline = false;

    
    let reason = AddonManager.UPDATE_WHEN_PERIODIC_UPDATE;
    if (!aTimer)
      reason = AddonManager.UPDATE_WHEN_USER_REQUESTED;

    AddonManager.getAddonsByTypes(null, function(aAddonList) {
      aAddonList.forEach(function(aAddon) {
        if (aAddon.permissions & AddonManager.PERM_CAN_UPGRADE) {
          let data = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
          data.data = JSON.stringify({ id: aAddon.id, name: aAddon.name });
          Services.obs.notifyObservers(data, "addon-update-started", null);

          let listener = new UpdateCheckListener();
          aAddon.findUpdates(listener, reason);
        }
      });
    });

    RecommendedSearchResults.search();
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

    Services.obs.notifyObservers(data, "addon-update-ended", this._status);
  }
};





var RecommendedSearchResults = {
  _getFile: function() {
    let dirService = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
    let file = dirService.get("ProfD", Ci.nsILocalFile);
    file.append("recommended-addons.json");
    return file;
  },

  _writeFile: function (aFile, aData) {
    if (!aData)
      return;

    
    let array = new TextEncoder().encode(aData);
    OS.File.writeAtomic(aFile.path, array, { tmpPath: aFile.path + ".tmp" }).then(function onSuccess() {
      Services.obs.notifyObservers(null, "recommended-addons-cache-updated", "");
    });
  },
  
  searchSucceeded: function(aAddons, aAddonCount, aTotalResults) {
    let self = this;

    
    AddonManager.getAllAddons(function(aAllAddons) {
      let addons = aAddons.filter(function(addon) {
        for (let i = 0; i < aAllAddons.length; i++)
          if (addon.id == aAllAddons[i].id)
            return false;

        return true;
      });

      let json = {
        addons: []
      };

      addons.forEach(function(aAddon) {
        json.addons.push({
          id: aAddon.id,
          name: aAddon.name,
          version: aAddon.version,
          learnmoreURL: aAddon.learnmoreURL,
          iconURL: aAddon.iconURL
        })
      });

      let file = self._getFile();
      self._writeFile(file, JSON.stringify(json));
    });
  },
  
  searchFailed: function searchFailed() { },
  
  search: function() {
    const kAddonsMaxDisplay = 2;

    if (AddonRepository.isSearching)
      AddonRepository.cancelSearch();
    AddonRepository.retrieveRecommendedAddons(kAddonsMaxDisplay, RecommendedSearchResults);
  }
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([AddonUpdateService]);

