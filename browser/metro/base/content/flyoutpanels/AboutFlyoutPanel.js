



'use strict';

Cu.import("resource://gre/modules/Services.jsm");
let gAppUpdater;

let AboutFlyoutPanel = {
  init: function() {
    if (this._isInitialized) {
      Cu.reportError("Attempted to initialize AboutFlyoutPanel more than once");
    }

    this._isInitialized = true;

    let self = this;
    this._elements = {};
    [
      ['versionLabel', 'about-version-label'],
      ['AboutFlyoutPanel',  'about-flyoutpanel'],
    ].forEach(function(aElement) {
      let [name, id] = aElement;
      XPCOMUtils.defineLazyGetter(self._elements, name, function() {
        return document.getElementById(id);
      });
    });

    this._topmostElement = this._elements.AboutFlyoutPanel;

    
    let version = Services.appinfo.version;
    if (/a\d+$/.test(version)) {
      let buildID = Services.appinfo.appBuildID;
      let buildDate = buildID.slice(0,4) + "-" + buildID.slice(4,6) +
                      "-" + buildID.slice(6,8);
      this._elements.versionLabel.textContent +=" (" + buildDate + ")";
    }

    window.addEventListener('MozFlyoutPanelShowing', this, false);

#if MOZ_UPDATE_CHANNEL != release
    let defaults = Services.prefs.getDefaultBranch("");
    let channelLabel = document.getElementById("currentChannel");
    channelLabel.value = defaults.getCharPref("app.update.channel");
#endif
  },

  onPolicyClick: function(aEvent) {
    if (aEvent.button != 0) {
      return;
    }
    let url = Services.urlFormatter.formatURLPref("app.privacyURL");
    BrowserUI.addAndShowTab(url, Browser.selectedTab);
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case 'MozFlyoutPanelShowing':
#ifdef MOZ_UPDATER
        onUnload();
        this.appUpdater = new appUpdater();
        gAppUpdater = this.appUpdater;
#endif
        break;
    }
  }
};

#ifdef MOZ_UPDATER
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/DownloadUtils.jsm");
Components.utils.import("resource://gre/modules/AddonManager.jsm");

function onUnload(aEvent) {
  if (!gAppUpdater) {
    return;
  }

  if (gAppUpdater.isChecking)
    gAppUpdater.checker.stopChecking(Components.interfaces.nsIUpdateChecker.CURRENT_CHECK);
  
  gAppUpdater.removeDownloadListener();
  gAppUpdater = null;
  AboutFlyout.appUpdater = null;
}

function appUpdater()
{
  this.updateDeck = document.getElementById("updateDeck");

  XPCOMUtils.defineLazyServiceGetter(this, "aus",
                                     "@mozilla.org/updates/update-service;1",
                                     "nsIApplicationUpdateService");
  XPCOMUtils.defineLazyServiceGetter(this, "checker",
                                     "@mozilla.org/updates/update-checker;1",
                                     "nsIUpdateChecker");
  XPCOMUtils.defineLazyServiceGetter(this, "um",
                                     "@mozilla.org/updates/update-manager;1",
                                     "nsIUpdateManager");

  this.bundle = Services.strings.
                createBundle("chrome://browser/locale/browser.properties");

  this.updateBtn = document.getElementById("updateButton");

  
  this.setupUpdateButton("update.checkInsideButton");

  let manualURL = Services.urlFormatter.formatURLPref("app.update.url.manual");
  let manualLink = document.getElementById("manualLink");
  manualLink.value = manualURL;
  manualLink.href = manualURL;
  document.getElementById("failedLink").href = manualURL;

  if (this.updateDisabledAndLocked) {
    this.selectPanel("adminDisabled");
    return;
  }

  if (this.isPending || this.isApplied) {
    this.setupUpdateButton("update.restart." +
                           (this.isMajor ? "upgradeButton" : "updateButton"));
    return;
  }

  if (this.aus.isOtherInstanceHandlingUpdates) {
    this.selectPanel("otherInstanceHandlingUpdates");
    return;
  }

  if (this.isDownloading) {
    this.startDownload();
    return;
  }

  if (this.updateEnabled && this.updateAuto) {
    this.selectPanel("checkingForUpdates");
    this.isChecking = true;
    this.checker.checkForUpdates(this.updateCheckListener, true);
    return;
  }
}

appUpdater.prototype =
{
  
  isChecking: false,

  
  get isPending() {
    if (this.update) {
      return this.update.state == "pending" ||
             this.update.state == "pending-service";
    }
    return this.um.activeUpdate &&
           (this.um.activeUpdate.state == "pending" ||
            this.um.activeUpdate.state == "pending-service");
  },

  
  get isApplied() {
    if (this.update)
      return this.update.state == "applied" ||
             this.update.state == "applied-service";
    return this.um.activeUpdate &&
           (this.um.activeUpdate.state == "applied" ||
            this.um.activeUpdate.state == "applied-service");
  },

  
  get isDownloading() {
    if (this.update)
      return this.update.state == "downloading";
    return this.um.activeUpdate &&
           this.um.activeUpdate.state == "downloading";
  },

  
  get isMajor() {
    if (this.update)
      return this.update.type == "major";
    return this.um.activeUpdate.type == "major";
  },

  
  get updateDisabledAndLocked() {
    return !this.updateEnabled &&
           Services.prefs.prefIsLocked("app.update.enabled");
  },

  
  get updateEnabled() {
    let updatesEnabled = true;
    try {
      updatesEnabled = Services.prefs.getBoolPref("app.update.metro.enabled");
    }
    catch (e) { }
    if (!updatesEnabled) {
      return false;
    }

    try {
      updatesEnabled = Services.prefs.getBoolPref("app.update.enabled")
    }
    catch (e) { }

    return updatesEnabled;
  },

  
  get backgroundUpdateEnabled() {
    return this.updateEnabled &&
           gAppUpdater.aus.canStageUpdates;
  },

  
  get updateAuto() {
    try {
      return Services.prefs.getBoolPref("app.update.auto");
    }
    catch (e) { }
    return true; 
  },

  





  selectPanel: function(aChildID) {
    this.updateDeck.selectedPanel = document.getElementById(aChildID);
    this.updateBtn.disabled = (aChildID != "updateButtonBox");
  },

  






  setupUpdateButton: function(aKeyPrefix) {
    this.updateBtn.label = this.bundle.GetStringFromName(aKeyPrefix + ".label");
    this.updateBtn.accessKey = this.bundle.GetStringFromName(aKeyPrefix + ".accesskey");
    if (!document.commandDispatcher.focusedElement ||
        document.commandDispatcher.focusedElement == this.updateBtn)
      this.updateBtn.focus();
  },

  


  buttonOnCommand: function() {
    if (this.isPending || this.isApplied) {
      
      let cancelQuit = Components.classes["@mozilla.org/supports-PRBool;1"].
                       createInstance(Components.interfaces.nsISupportsPRBool);
      Services.obs.notifyObservers(cancelQuit, "quit-application-requested", "restart");

      
      if (cancelQuit.data)
        return;

      let appStartup = Components.classes["@mozilla.org/toolkit/app-startup;1"].
                       getService(Components.interfaces.nsIAppStartup);

      
      if (Services.appinfo.inSafeMode) {
        appStartup.restartInSafeMode(Components.interfaces.nsIAppStartup.eAttemptQuit);
        return;
      }

      appStartup.quit(Components.interfaces.nsIAppStartup.eAttemptQuit |
                      Components.interfaces.nsIAppStartup.eRestart);
      return;
    }

    const URI_UPDATE_PROMPT_DIALOG = "chrome://mozapps/content/update/updates.xul";
    
    
    if (this.update && (this.update.billboardURL || this.update.licenseURL ||
        this.addons.length != 0)) {
      var ary = null;
      ary = Components.classes["@mozilla.org/supports-array;1"].
            createInstance(Components.interfaces.nsISupportsArray);
      ary.AppendElement(this.update);
      var openFeatures = "chrome,centerscreen,dialog=no,resizable=no,titlebar,toolbar=no";
      Services.ww.openWindow(null, URI_UPDATE_PROMPT_DIALOG, "", openFeatures, ary);
      window.close();
      return;
    }

    this.selectPanel("checkingForUpdates");
    this.isChecking = true;
    this.checker.checkForUpdates(this.updateCheckListener, true);
  },

  




  updateCheckListener: {
    


    onCheckComplete: function(aRequest, aUpdates, aUpdateCount) {
      gAppUpdater.isChecking = false;
      gAppUpdater.update = gAppUpdater.aus.
                           selectUpdate(aUpdates, aUpdates.length);
      if (!gAppUpdater.update) {
        gAppUpdater.selectPanel("noUpdatesFound");
        return;
      }

      if (!gAppUpdater.aus.canApplyUpdates) {
        gAppUpdater.selectPanel("manualUpdate");
        return;
      }

      
      
      if (gAppUpdater.update.billboardURL || gAppUpdater.update.licenseURL) {
        gAppUpdater.selectPanel("updateButtonBox");
        gAppUpdater.setupUpdateButton("update.openUpdateUI." +
                                      (this.isMajor ? "upgradeButton"
                                                    : "applyButton"));
        return;
      }

      if (!gAppUpdater.update.appVersion ||
          Services.vc.compare(gAppUpdater.update.appVersion,
                              Services.appinfo.version) == 0) {
        gAppUpdater.startDownload();
        return;
      }

      gAppUpdater.checkAddonCompatibility();
    },

    


    onError: function(aRequest, aUpdate) {
      
      
      
      gAppUpdater.isChecking = false;
      gAppUpdater.selectPanel("noUpdatesFound");
    },

    


    QueryInterface: function(aIID) {
      if (!aIID.equals(Components.interfaces.nsIUpdateCheckListener) &&
          !aIID.equals(Components.interfaces.nsISupports))
        throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
    }
  },

  


  checkAddonCompatibility: function() {
    try {
      var hotfixID = Services.prefs.getCharPref(PREF_EM_HOTFIX_ID);
    }
    catch (e) { }

    var self = this;
    AddonManager.getAllAddons(function(aAddons) {
      self.addons = [];
      self.addonsCheckedCount = 0;
      aAddons.forEach(function(aAddon) {
        
        
        if (!("isCompatibleWith" in aAddon) || !("findUpdates" in aAddon)) {
          let errMsg = "Add-on doesn't implement either the isCompatibleWith " +
                       "or the findUpdates method!";
          if (aAddon.id)
            errMsg += " Add-on ID: " + aAddon.id;
          Components.utils.reportError(errMsg);
          return;
        }

        
        
        
        
        
        
        
        
        
        
        try {
          if (aAddon.type != "plugin" && aAddon.id != hotfixID &&
              !aAddon.appDisabled && !aAddon.userDisabled &&
              aAddon.scope != AddonManager.SCOPE_APPLICATION &&
              aAddon.isCompatible &&
              !aAddon.isCompatibleWith(self.update.appVersion,
                                       self.update.platformVersion))
            self.addons.push(aAddon);
        }
        catch (e) {
          Components.utils.reportError(e);
        }
      });
      self.addonsTotalCount = self.addons.length;
      if (self.addonsTotalCount == 0) {
        self.startDownload();
        return;
      }

      self.checkAddonsForUpdates();
    });
  },

  



  checkAddonsForUpdates: function() {
    this.addons.forEach(function(aAddon) {
      aAddon.findUpdates(this, AddonManager.UPDATE_WHEN_NEW_APP_DETECTED,
                         this.update.appVersion,
                         this.update.platformVersion);
    }, this);
  },

  


  onCompatibilityUpdateAvailable: function(aAddon) {
    for (var i = 0; i < this.addons.length; ++i) {
      if (this.addons[i].id == aAddon.id) {
        this.addons.splice(i, 1);
        break;
      }
    }
  },

  


  onUpdateAvailable: function(aAddon, aInstall) {
    if (!Services.blocklist.isAddonBlocklisted(aAddon.id, aInstall.version,
                                               this.update.appVersion,
                                               this.update.platformVersion)) {
      
      this.onCompatibilityUpdateAvailable(aAddon);
    }
  },

  


  onUpdateFinished: function(aAddon) {
    ++this.addonsCheckedCount;

    if (this.addonsCheckedCount < this.addonsTotalCount)
      return;

    if (this.addons.length == 0) {
      
      this.startDownload();
      return;
    }

    this.selectPanel("updateButtonBox");
    this.setupUpdateButton("update.openUpdateUI." +
                           (this.isMajor ? "upgradeButton" : "applyButton"));
  },

  


  startDownload: function() {
    if (!this.update)
      this.update = this.um.activeUpdate;
    this.update.QueryInterface(Components.interfaces.nsIWritablePropertyBag);
    this.update.setProperty("foregroundDownload", "true");

    this.aus.pauseDownload();
    let state = this.aus.downloadUpdate(this.update, false);
    if (state == "failed") {
      this.selectPanel("downloadFailed");
      return;
    }

    this.setupDownloadingUI();
  },

  


  setupDownloadingUI: function() {
    this.downloadStatus = document.getElementById("downloadStatus");
    this.downloadStatus.value =
      DownloadUtils.getTransferTotal(0, this.update.selectedPatch.size);
    this.selectPanel("downloading");
    this.aus.addDownloadListener(this);
  },

  removeDownloadListener: function() {
    if (this.aus) {
      this.aus.removeDownloadListener(this);
    }
  },

  


  onStartRequest: function(aRequest, aContext) {
  },

  


  onStopRequest: function(aRequest, aContext, aStatusCode) {
    switch (aStatusCode) {
    case Components.results.NS_ERROR_UNEXPECTED:
      if (this.update.selectedPatch.state == "download-failed" &&
          (this.update.isCompleteUpdate || this.update.patchCount != 2)) {
        
        
        this.removeDownloadListener();
        this.selectPanel("downloadFailed");
        break;
      }
      
      
      break;
    case Components.results.NS_BINDING_ABORTED:
      
      break;
    case Components.results.NS_OK:
      this.removeDownloadListener();
      if (this.backgroundUpdateEnabled) {
        this.selectPanel("applying");
        let update = this.um.activeUpdate;
        let self = this;
        Services.obs.addObserver(function updateStaged(aSubject, aTopic, aData) {
          
          let status = aData;
          if (status == "applied" || status == "applied-service" ||
              status == "pending" || status == "pending-service") {
            
            
            
            self.selectPanel("updateButtonBox");
            self.setupUpdateButton("update.restart." +
                                   (self.isMajor ? "upgradeButton" : "updateButton"));
          } else if (status == "failed") {
            
            
            self.selectPanel("downloadFailed");
          } else if (status == "downloading") {
            
            
            
            self.setupDownloadingUI();
            return;
          }
          Services.obs.removeObserver(updateStaged, "update-staged");
        }, "update-staged", false);
      } else {
        this.selectPanel("updateButtonBox");
        this.setupUpdateButton("update.restart." +
                               (this.isMajor ? "upgradeButton" : "updateButton"));
      }
      break;
    default:
      this.removeDownloadListener();
      this.selectPanel("downloadFailed");
      break;
    }

  },

  


  onStatus: function(aRequest, aContext, aStatus, aStatusArg) {
  },

  


  onProgress: function(aRequest, aContext, aProgress, aProgressMax) {
    this.downloadStatus.value =
      DownloadUtils.getTransferTotal(aProgress, aProgressMax);
  },

  


  QueryInterface: function(aIID) {
    if (!aIID.equals(Components.interfaces.nsIProgressEventSink) &&
        !aIID.equals(Components.interfaces.nsIRequestObserver) &&
        !aIID.equals(Components.interfaces.nsISupports))
      throw Components.results.NS_ERROR_NO_INTERFACE;
    return this;
  }
};
#endif
