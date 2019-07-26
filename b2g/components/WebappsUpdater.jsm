



"use strict";

this.EXPORTED_SYMBOLS = ["WebappsUpdater"];

const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

this.WebappsUpdater = {
  _checkingApps: false,
  _pendingEvents: [],

  handleContentStart: function(aShell) {
    let content = aShell.contentBrowser.contentWindow;
    this._pendingEvents.forEach(aShell.sendChromeEvent);

    this._pendingEvents.length = 0;
  },

  sendChromeEvent: function(aType, aDetail) {
    let detail = aDetail || {};
    detail.type = aType;

    let browser = Services.wm.getMostRecentWindow("navigator:browser");
    if (!browser) {
      this._pendingEvents.push(detail);
      dump("Warning: Couldn't send update event " + aType +
          ": no content browser. Will send again when content becomes available.");
      return false;
    }

    browser.shell.sendChromeEvent(detail);
    return true;
  },

  _appsUpdated: function(aApps) {
    dump("appsUpdated: " + aApps.length + " apps to update");
    let lock = Services.settings.createLock();
    lock.set("apps.updateStatus", "check-complete", null);
    this.sendChromeEvent("apps-update-check", { apps: aApps });
    this._checkingApps = false;
  },

  
  
  updateApps: function() {
    dump("updateApps (" + this._checkingApps + ")");
    
    if (this._checkingApps) {
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
