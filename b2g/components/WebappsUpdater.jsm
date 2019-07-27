



"use strict";

this.EXPORTED_SYMBOLS = ["WebappsUpdater"];

const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "settings",
                                   "@mozilla.org/settingsService;1",
                                   "nsISettingsService");

XPCOMUtils.defineLazyModuleGetter(this, "SystemAppProxy",
                                  "resource://gre/modules/SystemAppProxy.jsm");

function debug(aStr) {
  
}

this.WebappsUpdater = {
  _checkingApps: false,

  handleContentStart: function() {
  },

  sendChromeEvent: function(aType, aDetail) {
    let detail = aDetail || {};
    detail.type = aType;

    let sent = SystemAppProxy.dispatchEvent(detail);
    if (!sent) {
      debug("Warning: Couldn't send update event " + aType +
          ": no content browser. Will send again when content becomes available.");
      return false;
    }

    return true;
  },

  _appsUpdated: function(aApps) {
    debug("appsUpdated: " + aApps.length + " apps to update");
    let lock = settings.createLock();
    lock.set("apps.updateStatus", "check-complete", null);
    this.sendChromeEvent("apps-update-check", { apps: aApps });
    this._checkingApps = false;
  },

  
  
  updateApps: function() {
    debug("updateApps (" + this._checkingApps + ")");
    
    if (this._checkingApps) {
      return;
    }

    let allowUpdate = true;
    try {
      allowUpdate = Services.prefs.getBoolPref("webapps.update.enabled");
    } catch (ex) { }

    if (!allowUpdate) {
      return;
    }

    this._checkingApps = true;

    let self = this;

    let window = Services.wm.getMostRecentWindow("navigator:browser");
    let all = window.navigator.mozApps.mgmt.getAll();

    all.onsuccess = function() {
      let appsCount = this.result.length;
      let appsChecked = 0;
      let appsToUpdate = [];
      this.result.forEach(function updateApp(aApp) {
        let update = aApp.checkForUpdate();
        update.onsuccess = function() {
          if (aApp.downloadAvailable) {
            appsToUpdate.push(aApp.manifestURL);
          }

          appsChecked += 1;
          if (appsChecked == appsCount) {
            self._appsUpdated(appsToUpdate);
          }
        }
        update.onerror = function() {
          appsChecked += 1;
          if (appsChecked == appsCount) {
            self._appsUpdated(appsToUpdate);
          }
        }
      });
    }

    all.onerror = function() {
      
      self._appsUpdated([]);
    }
  }
};
