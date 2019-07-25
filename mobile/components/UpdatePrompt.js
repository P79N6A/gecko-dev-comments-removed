




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const UPDATE_NOTIFICATION_NAME = "update-app";
const UPDATE_NOTIFICATION_ICON = "drawable://alert_download_progress";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");


XPCOMUtils.defineLazyGetter(this, "gUpdateBundle", function aus_gUpdateBundle() {
  return Services.strings.createBundle("chrome://mozapps/locale/update/updates.properties");
});

XPCOMUtils.defineLazyGetter(this, "gBrandBundle", function aus_gBrandBundle() {
  return Services.strings.createBundle("chrome://branding/locale/brand.properties");
});

XPCOMUtils.defineLazyGetter(this, "gBrowserBundle", function aus_gBrowserBundle() {
  return Services.strings.createBundle("chrome://browser/locale/browser.properties");
});

function getPref(func, preference, defaultValue) {
  try {
    return Services.prefs[func](preference);
  } catch (e) {}
  return defaultValue;
}





function UpdatePrompt() { }

UpdatePrompt.prototype = {
  classID: Components.ID("{88b3eb21-d072-4e3b-886d-f89d8c49fe59}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdatePrompt, Ci.nsIRequestObserver, Ci.nsIProgressEventSink]),

  get _enabled() {
    return !getPref("getBoolPref", "app.update.silent", false);
  },

  _showNotification: function UP__showNotif(aUpdate, aTitle, aText, aImageUrl, aMode) {
    let observer = {
      updatePrompt: this,
      observe: function (aSubject, aTopic, aData) {
        switch (aTopic) {
          case "alertclickcallback":
            this.updatePrompt._handleUpdate(aUpdate, aMode);
            break;
        }
      }
    };

    let notifier = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
    notifier.showAlertNotification(aImageUrl, aTitle, aText, true, "", observer, UPDATE_NOTIFICATION_NAME);
  },

  _handleUpdate: function UP__handleUpdate(aUpdate, aMode) {
    if (aMode == "available") {
      let window = Services.wm.getMostRecentWindow("navigator:browser");
      let title = gUpdateBundle.GetStringFromName("updatesfound_" + aUpdate.type + ".title");
      let brandName = gBrandBundle.GetStringFromName("brandShortName");

      
      
      let message = gUpdateBundle.formatStringFromName("intro_major", [brandName, aUpdate.displayVersion], 2);
      let button0 = gUpdateBundle.GetStringFromName("updateButton_major");
      let button1 = gUpdateBundle.GetStringFromName("askLaterButton");
      let prompt = Services.prompt;
      let flags = prompt.BUTTON_POS_0 * prompt.BUTTON_TITLE_IS_STRING + prompt.BUTTON_POS_1 * prompt.BUTTON_TITLE_IS_STRING;

      let download = (prompt.confirmEx(window, title, message, flags, button0, button1, null, null, {value: false}) == 0);
      if (download) {
        
        let aus = Cc["@mozilla.org/updates/update-service;1"].getService(Ci.nsIApplicationUpdateService);
        if (aus.downloadUpdate(aUpdate, true) != "failed") {
          let title = gBrowserBundle.formatStringFromName("alertDownloadsStart", [aUpdate.name], 1);
          this._showNotification(aUpdate, title, "", UPDATE_NOTIFICATION_ICON, "download");

          
          aus.addDownloadListener(this);
        }
      }
    } else if(aMode == "downloaded") {
      
      let cancelQuit = Cc["@mozilla.org/supports-PRBool;1"].createInstance(Ci.nsISupportsPRBool);
      Services.obs.notifyObservers(cancelQuit, "quit-application-requested", "restart");

      
      if (cancelQuit.data == false) {
        let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
        appStartup.quit(Ci.nsIAppStartup.eRestart | Ci.nsIAppStartup.eAttemptQuit);
      }
    }
  },

  _updateDownloadProgress: function UP__updateDownloadProgress(aProgress, aTotal) {
    let alertsService = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
    let progressListener = alertsService.QueryInterface(Ci.nsIAlertsProgressListener);
    if (progressListener)
      progressListener.onProgress(UPDATE_NOTIFICATION_NAME, aProgress, aTotal);
  },

  
  
  

  
  checkForUpdates: function UP_checkForUpdates() {
    if (!this._enabled)
      return;

    let aus = Cc["@mozilla.org/updates/update-service;1"].getService(Ci.nsIApplicationUpdateService);
    if (!aus.isDownloading)
      return;

    let updateManager = Cc["@mozilla.org/updates/update-manager;1"].getService(Ci.nsIUpdateManager);

    let updateName = updateManager.activeUpdate ? updateManager.activeUpdate.name : gBrandBundle.GetStringFromName("brandShortName");
    let title = gBrowserBundle.formatStringFromName("alertDownloadsStart", [updateName], 1);

    this._showNotification(updateManager.activeUpdate, title, "", UPDATE_NOTIFICATION_ICON, "downloading");

    aus.removeDownloadListener(this); 
    aus.addDownloadListener(this);
  },

  showUpdateAvailable: function UP_showUpdateAvailable(aUpdate) {
    if (!this._enabled)
      return;

    let stringsPrefix = "updateAvailable_" + aUpdate.type + ".";
    let title = gUpdateBundle.formatStringFromName(stringsPrefix + "title", [aUpdate.name], 1);
    let text = gUpdateBundle.GetStringFromName(stringsPrefix + "text");
    let imageUrl = "";
    this._showNotification(aUpdate, title, text, imageUrl, "available");
  },

  showUpdateDownloaded: function UP_showUpdateDownloaded(aUpdate, aBackground) {
    if (!this._enabled)
      return;

    let stringsPrefix = "updateDownloaded_" + aUpdate.type + ".";
    let title = gUpdateBundle.formatStringFromName(stringsPrefix + "title", [aUpdate.name], 1);
    let text = gUpdateBundle.GetStringFromName(stringsPrefix + "text");
    let imageUrl = "";
    this._showNotification(aUpdate, title, text, imageUrl, "downloaded");
  },

  showUpdateInstalled: function UP_showUpdateInstalled() {
    if (!this._enabled || !getPref("getBoolPref", "app.update.showInstalledUI", false))
      return;

    let title = gBrandBundle.GetStringFromName("brandShortName");
    let text = gUpdateBundle.GetStringFromName("installSuccess");
    let imageUrl = "";
    this._showNotification(aUpdate, title, text, imageUrl, "installed");
  },

  showUpdateError: function UP_showUpdateError(aUpdate) {
    if (!this._enabled)
      return;

    if (aUpdate.state == "failed") {
      var title = gBrandBundle.GetStringFromName("brandShortName");
      let text = gUpdateBundle.GetStringFromName("updaterIOErrorTitle");
      let imageUrl = "";
      this._showNotification(aUpdate, title, text, imageUrl, "error");
    }
  },

  showUpdateHistory: function UP_showUpdateHistory(aParent) {
    
  },
  
  
  
  
  
  
  onStartRequest: function(request, context) {
    
  },

  
  onStopRequest: function(request, context, status) {
    let alertsService = Cc["@mozilla.org/alerts-service;1"].getService(Ci.nsIAlertsService);
    let progressListener = alertsService.QueryInterface(Ci.nsIAlertsProgressListener);
    if (progressListener)
      progressListener.onCancel(UPDATE_NOTIFICATION_NAME);


    let aus = Cc["@mozilla.org/updates/update-service;1"].getService(Ci.nsIApplicationUpdateService);
    aus.removeDownloadListener(this);
  },

  
  
  
  
  
  onProgress: function(request, context, progress, maxProgress) {
    this._updateDownloadProgress(progress, maxProgress);
  },

  
  onStatus: function(request, context, status, statusText) {
    
  }
  
};

const NSGetFactory = XPCOMUtils.generateNSGetFactory([UpdatePrompt]);
