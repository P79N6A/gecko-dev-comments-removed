







"use strict";

const PREF_UPDATE_EXTENSIONS_ENABLED            = "extensions.update.enabled";
const PREF_XPINSTALL_ENABLED                    = "xpinstall.enabled";


const METADATA_TIMEOUT    = 30000;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AddonManager", "resource://gre/modules/AddonManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AddonManagerPrivate", "resource://gre/modules/AddonManager.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Services", "resource://gre/modules/Services.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "AddonRepository", "resource://gre/modules/addons/AddonRepository.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task", "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise", "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Log", "resource://gre/modules/Log.jsm");
let logger = null;

var gUpdateWizard = {
  
  
  
  addons: [],
  
  affectedAddonIDs: null,
  
  addonsToUpdate: [],
  shouldSuggestAutoChecking: false,
  shouldAutoCheck: false,
  xpinstallEnabled: true,
  xpinstallLocked: false,
  
  
  addonInstalls: new Map(),
  shuttingDown: false,
  
  
  disabled: 0,
  metadataEnabled: 0,
  metadataDisabled: 0,
  upgraded: 0,
  upgradeFailed: 0,
  upgradeDeclined: 0,

  init: function gUpdateWizard_init()
  {
    logger = Log.repository.getLogger("addons.update-dialog");
    
    this.affectedAddonIDs = new Set(window.arguments[0]);

    try {
      this.shouldSuggestAutoChecking =
        !Services.prefs.getBoolPref(PREF_UPDATE_EXTENSIONS_ENABLED);
    }
    catch (e) {
    }

    try {
      this.xpinstallEnabled = Services.prefs.getBoolPref(PREF_XPINSTALL_ENABLED);
      this.xpinstallLocked = Services.prefs.prefIsLocked(PREF_XPINSTALL_ENABLED);
    }
    catch (e) {
    }

    if (Services.io.offline)
      document.documentElement.currentPage = document.getElementById("offline");
    else
      document.documentElement.currentPage = document.getElementById("versioninfo");
  },

  onWizardFinish: function gUpdateWizard_onWizardFinish ()
  {
    if (this.shouldSuggestAutoChecking)
      Services.prefs.setBoolPref(PREF_UPDATE_EXTENSIONS_ENABLED, this.shouldAutoCheck);
  },

  _setUpButton: function gUpdateWizard_setUpButton(aButtonID, aButtonKey, aDisabled)
  {
    var strings = document.getElementById("updateStrings");
    var button = document.documentElement.getButton(aButtonID);
    if (aButtonKey) {
      button.label = strings.getString(aButtonKey);
      try {
        button.setAttribute("accesskey", strings.getString(aButtonKey + "Accesskey"));
      }
      catch (e) {
      }
    }
    button.disabled = aDisabled;
  },

  setButtonLabels: function gUpdateWizard_setButtonLabels(aBackButton, aBackButtonIsDisabled,
                             aNextButton, aNextButtonIsDisabled,
                             aCancelButton, aCancelButtonIsDisabled)
  {
    this._setUpButton("back", aBackButton, aBackButtonIsDisabled);
    this._setUpButton("next", aNextButton, aNextButtonIsDisabled);
    this._setUpButton("cancel", aCancelButton, aCancelButtonIsDisabled);
  },

  
  
  errorItems: [],

  checkForErrors: function gUpdateWizard_checkForErrors(aElementIDToShow)
  {
    if (this.errorItems.length > 0)
      document.getElementById(aElementIDToShow).hidden = false;
  },

  onWizardClose: function gUpdateWizard_onWizardClose(aEvent)
  {
    return this.onWizardCancel();
  },

  onWizardCancel: function gUpdateWizard_onWizardCancel()
  {
    gUpdateWizard.shuttingDown = true;
    
    
    
    
    if (gMismatchPage.waiting) {
      logger.info("Dialog closed in mismatch page");
      if (gUpdateWizard.addonInstalls.size > 0) {
        gInstallingPage.startInstalls([i for ([, i] of gUpdateWizard.addonInstalls)]);
      }
      return true;
    }

    
    
    if (!gInstallingPage.installing) {
      logger.info("Dialog closed while waiting for updated compatibility information");
    }
    else {
      logger.info("Dialog closed while downloading and installing updates");
    }
    return true;
  }
};

var gOfflinePage = {
  onPageAdvanced: function gOfflinePage_onPageAdvanced()
  {
    Services.io.offline = false;
    return true;
  },

  toggleOffline: function gOfflinePage_toggleOffline()
  {
    var nextbtn = document.documentElement.getButton("next");
    nextbtn.disabled = !nextbtn.disabled;
  }
}


let listener = {
  onDisabled: function listener_onDisabled(aAddon) {
    gUpdateWizard.affectedAddonIDs.add(aAddon.id);
    gUpdateWizard.metadataDisabled++;
  },
  onEnabled: function listener_onEnabled(aAddon) {
    gUpdateWizard.affectedAddonIDs.delete(aAddon.id);
    gUpdateWizard.metadataEnabled++;
  }
};

var gVersionInfoPage = {
  _completeCount: 0,
  _totalCount: 0,
  _versionInfoDone: false,
  onPageShow: Task.async(function* gVersionInfoPage_onPageShow() {
    gUpdateWizard.setButtonLabels(null, true,
                                  "nextButtonText", true,
                                  "cancelButtonText", false);

    gUpdateWizard.disabled = gUpdateWizard.affectedAddonIDs.size;

    
    
    AddonManager.addAddonListener(listener);
    if (AddonRepository.isMetadataStale()) {
      
      yield AddonRepository.repopulateCache(METADATA_TIMEOUT);
      if (gUpdateWizard.shuttingDown) {
        logger.debug("repopulateCache completed after dialog closed");
      }
    }
    
    
    let idlist = [id for (id of gUpdateWizard.affectedAddonIDs)
                     if (id != AddonManager.hotfixID)];
    if (idlist.length < 1) {
      gVersionInfoPage.onAllUpdatesFinished();
      return;
    }

    logger.debug("Fetching affected addons " + idlist.toSource());
    let fetchedAddons = yield new Promise((resolve, reject) =>
      AddonManager.getAddonsByIDs(idlist, resolve));
    
    gUpdateWizard.addons = [a for (a of fetchedAddons) if (a)];
    if (gUpdateWizard.addons.length < 1) {
      gVersionInfoPage.onAllUpdatesFinished();
      return;
    }

    gVersionInfoPage._totalCount = gUpdateWizard.addons.length;

    for (let addon of gUpdateWizard.addons) {
      logger.debug("VersionInfo Finding updates for ${id}", addon);
      addon.findUpdates(gVersionInfoPage, AddonManager.UPDATE_WHEN_NEW_APP_INSTALLED);
    }
  }),

  onAllUpdatesFinished: function gVersionInfoPage_onAllUpdatesFinished() {
    AddonManager.removeAddonListener(listener);
    AddonManagerPrivate.recordSimpleMeasure("appUpdate_disabled",
        gUpdateWizard.disabled);
    AddonManagerPrivate.recordSimpleMeasure("appUpdate_metadata_enabled",
        gUpdateWizard.metadataEnabled);
    AddonManagerPrivate.recordSimpleMeasure("appUpdate_metadata_disabled",
        gUpdateWizard.metadataDisabled);
    
    
    AddonManagerPrivate.recordSimpleMeasure("appUpdate_upgraded", 0);
    AddonManagerPrivate.recordSimpleMeasure("appUpdate_upgradeFailed", 0);
    AddonManagerPrivate.recordSimpleMeasure("appUpdate_upgradeDeclined", 0);
    
    logger.debug("VersionInfo updates finished: found " +
         [addon.id + ":" + addon.appDisabled for (addon of gUpdateWizard.addons)].toSource());
    let filteredAddons = [];
    for (let a of gUpdateWizard.addons) {
      if (a.appDisabled) {
        logger.debug("Continuing with add-on " + a.id);
        filteredAddons.push(a);
      }
      else if (gUpdateWizard.addonInstalls.has(a.id)) {
        gUpdateWizard.addonInstalls.get(a.id).cancel();
        gUpdateWizard.addonInstalls.delete(a.id);
      }
    }
    gUpdateWizard.addons = filteredAddons;

    if (gUpdateWizard.shuttingDown) {
      
      if (gUpdateWizard.addonInstalls.size > 0) {
        gInstallingPage.startInstalls([i for ([, i] of gUpdateWizard.addonInstalls)]);
      }
      return;
    }

    if (filteredAddons.length > 0) {
      if (!gUpdateWizard.xpinstallEnabled && gUpdateWizard.xpinstallLocked) {
        document.documentElement.currentPage = document.getElementById("adminDisabled");
        return;
      }
      document.documentElement.currentPage = document.getElementById("mismatch");
    }
    else {
      logger.info("VersionInfo: No updates require further action");
      
      
      
      
      setTimeout(close, 0);
    }
  },

  
  
  onUpdateFinished: function gVersionInfoPage_onUpdateFinished(aAddon, status) {
    ++this._completeCount;

    if (status != AddonManager.UPDATE_STATUS_NO_ERROR) {
      logger.debug("VersionInfo update " + this._completeCount + " of " + this._totalCount +
           " failed for " + aAddon.id + ": " + status);
      gUpdateWizard.errorItems.push(aAddon);
    }
    else {
      logger.debug("VersionInfo update " + this._completeCount + " of " + this._totalCount +
           " finished for " + aAddon.id);
    }

    
    
    if (!gUpdateWizard.shuttingDown) {
      
      
      if (aAddon.active) {
        AddonManagerPrivate.removeStartupChange(AddonManager.STARTUP_CHANGE_DISABLED, aAddon.id);
        gUpdateWizard.metadataEnabled++;
      }

      
      var updateStrings = document.getElementById("updateStrings");
      var statusElt = document.getElementById("versioninfo.status");
      var statusString = updateStrings.getFormattedString("statusPrefix", [aAddon.name]);
      statusElt.setAttribute("value", statusString);

      
      var progress = document.getElementById("versioninfo.progress");
      progress.mode = "normal";
      progress.value = Math.ceil((this._completeCount / this._totalCount) * 100);
    }

    if (this._completeCount == this._totalCount)
      this.onAllUpdatesFinished();
  },

  onUpdateAvailable: function gVersionInfoPage_onUpdateAvailable(aAddon, aInstall) {
    logger.debug("VersionInfo got an install for " + aAddon.id + ": " + aAddon.version);
    gUpdateWizard.addonInstalls.set(aAddon.id, aInstall);
  },
};

var gMismatchPage = {
  waiting: false,

  onPageShow: function gMismatchPage_onPageShow()
  {
    gMismatchPage.waiting = true;
    gUpdateWizard.setButtonLabels(null, true,
                                  "mismatchCheckNow", false,
                                  "mismatchDontCheck", false);
    document.documentElement.getButton("next").focus();

    var incompatible = document.getElementById("mismatch.incompatible");
    for (let addon of gUpdateWizard.addons) {
      var listitem = document.createElement("listitem");
      listitem.setAttribute("label", addon.name + " " + addon.version);
      incompatible.appendChild(listitem);
    }
  }
};

var gUpdatePage = {
  _totalCount: 0,
  _completeCount: 0,
  onPageShow: function gUpdatePage_onPageShow()
  {
    gMismatchPage.waiting = false;
    gUpdateWizard.setButtonLabels(null, true,
                                  "nextButtonText", true,
                                  "cancelButtonText", false);
    document.documentElement.getButton("next").focus();

    gUpdateWizard.errorItems = [];

    this._totalCount = gUpdateWizard.addons.length;
    for (let addon of gUpdateWizard.addons) {
      logger.debug("UpdatePage requesting update for " + addon.id);
      
      
      addon.findUpdates(this, AddonManager.UPDATE_WHEN_NEW_APP_INSTALLED);
    }
  },

  onAllUpdatesFinished: function gUpdatePage_onAllUpdatesFinished() {
    if (gUpdateWizard.shuttingDown)
      return;

    var nextPage = document.getElementById("noupdates");
    if (gUpdateWizard.addonsToUpdate.length > 0)
      nextPage = document.getElementById("found");
    document.documentElement.currentPage = nextPage;
  },

  
  
  onUpdateAvailable: function gUpdatePage_onUpdateAvailable(aAddon, aInstall) {
    logger.debug("UpdatePage got an update for " + aAddon.id + ": " + aAddon.version);
    gUpdateWizard.addonsToUpdate.push(aInstall);
  },

  onUpdateFinished: function gUpdatePage_onUpdateFinished(aAddon, status) {
    if (status != AddonManager.UPDATE_STATUS_NO_ERROR)
      gUpdateWizard.errorItems.push(aAddon);

    ++this._completeCount;

    if (!gUpdateWizard.shuttingDown) {
      
      var updateStrings = document.getElementById("updateStrings");
      var statusElt = document.getElementById("checking.status");
      var statusString = updateStrings.getFormattedString("statusPrefix", [aAddon.name]);
      statusElt.setAttribute("value", statusString);

      var progress = document.getElementById("checking.progress");
      progress.value = Math.ceil((this._completeCount / this._totalCount) * 100);
    }

    if (this._completeCount == this._totalCount)
      this.onAllUpdatesFinished()
  },
};

var gFoundPage = {
  onPageShow: function gFoundPage_onPageShow()
  {
    gUpdateWizard.setButtonLabels(null, true,
                                  "installButtonText", false,
                                  null, false);

    var foundUpdates = document.getElementById("found.updates");
    var itemCount = gUpdateWizard.addonsToUpdate.length;
    for (let install of gUpdateWizard.addonsToUpdate) {
      let listItem = foundUpdates.appendItem(install.name + " " + install.version);
      listItem.setAttribute("type", "checkbox");
      listItem.setAttribute("checked", "true");
      listItem.install = install;
    }

    if (!gUpdateWizard.xpinstallEnabled) {
      document.getElementById("xpinstallDisabledAlert").hidden = false;
      document.getElementById("enableXPInstall").focus();
      document.documentElement.getButton("next").disabled = true;
    }
    else {
      document.documentElement.getButton("next").focus();
      document.documentElement.getButton("next").disabled = false;
    }
  },

  toggleXPInstallEnable: function gFoundPage_toggleXPInstallEnable(aEvent)
  {
    var enabled = aEvent.target.checked;
    gUpdateWizard.xpinstallEnabled = enabled;
    var pref = Components.classes["@mozilla.org/preferences-service;1"]
                         .getService(Components.interfaces.nsIPrefBranch);
    pref.setBoolPref(PREF_XPINSTALL_ENABLED, enabled);
    this.updateNextButton();
  },

  updateNextButton: function gFoundPage_updateNextButton()
  {
    if (!gUpdateWizard.xpinstallEnabled) {
      document.documentElement.getButton("next").disabled = true;
      return;
    }

    var oneChecked = false;
    var foundUpdates = document.getElementById("found.updates");
    var updates = foundUpdates.getElementsByTagName("listitem");
    for (let update of updates) {
      if (!update.checked)
        continue;
      oneChecked = true;
      break;
    }

    gUpdateWizard.setButtonLabels(null, true,
                                  "installButtonText", true,
                                  null, false);
    document.getElementById("found").setAttribute("next", "installing");
    document.documentElement.getButton("next").disabled = !oneChecked;
  }
};

var gInstallingPage = {
  _installs         : [],
  _errors           : [],
  _strings          : null,
  _currentInstall   : -1,
  _installing       : false,

  
  
  startInstalls: function gInstallingPage_startInstalls(aInstallList) {
    if (!gUpdateWizard.xpinstallEnabled) {
      return;
    }

    logger.debug("Start installs for "
                 + [i.existingAddon.id for (i of aInstallList)].toSource());
    this._errors = [];
    this._installs = aInstallList;
    this._installing = true;
    this.startNextInstall();
  },

  onPageShow: function gInstallingPage_onPageShow()
  {
    gUpdateWizard.setButtonLabels(null, true,
                                  "nextButtonText", true,
                                  null, true);

    var foundUpdates = document.getElementById("found.updates");
    var updates = foundUpdates.getElementsByTagName("listitem");
    let toInstall = [];
    for (let update of updates) {
      if (!update.checked) {
        logger.info("User chose to cancel update of " + update.label);
        gUpdateWizard.upgradeDeclined++;
        update.install.cancel();
        continue;
      }
      toInstall.push(update.install);
    }
    this._strings = document.getElementById("updateStrings");

    this.startInstalls(toInstall);
  },

  startNextInstall: function gInstallingPage_startNextInstall() {
    if (this._currentInstall >= 0) {
      this._installs[this._currentInstall].removeListener(this);
    }

    this._currentInstall++;

    if (this._installs.length == this._currentInstall) {
      Services.obs.notifyObservers(null, "TEST:all-updates-done", null);
      AddonManagerPrivate.recordSimpleMeasure("appUpdate_upgraded",
          gUpdateWizard.upgraded);
      AddonManagerPrivate.recordSimpleMeasure("appUpdate_upgradeFailed",
          gUpdateWizard.upgradeFailed);
      AddonManagerPrivate.recordSimpleMeasure("appUpdate_upgradeDeclined",
          gUpdateWizard.upgradeDeclined);
      this._installing = false;
      if (gUpdateWizard.shuttingDown) {
        return;
      }
      var nextPage = this._errors.length > 0 ? "installerrors" : "finished";
      document.getElementById("installing").setAttribute("next", nextPage);
      document.documentElement.advance();
      return;
    }

    let install = this._installs[this._currentInstall];

    if (gUpdateWizard.shuttingDown && !AddonManager.shouldAutoUpdate(install.existingAddon)) {
      logger.debug("Don't update " + install.existingAddon.id + " in background");
      gUpdateWizard.upgradeDeclined++;
      install.cancel();
      this.startNextInstall();
      return;
    }
    install.addListener(this);
    install.install();
  },

  
  
  onDownloadStarted: function gInstallingPage_onDownloadStarted(aInstall) {
    if (gUpdateWizard.shuttingDown) {
      return;
    }
    var strings = document.getElementById("updateStrings");
    var label = strings.getFormattedString("downloadingPrefix", [aInstall.name]);
    var actionItem = document.getElementById("actionItem");
    actionItem.value = label;
  },

  onDownloadProgress: function gInstallingPage_onDownloadProgress(aInstall) {
    if (gUpdateWizard.shuttingDown) {
      return;
    }
    var downloadProgress = document.getElementById("downloadProgress");
    downloadProgress.value = Math.ceil(100 * aInstall.progress / aInstall.maxProgress);
  },

  onDownloadEnded: function gInstallingPage_onDownloadEnded(aInstall) {
  },

  onDownloadFailed: function gInstallingPage_onDownloadFailed(aInstall) {
    this._errors.push(aInstall);

    gUpdateWizard.upgradeFailed++;
    this.startNextInstall();
  },

  onInstallStarted: function gInstallingPage_onInstallStarted(aInstall) {
    if (gUpdateWizard.shuttingDown) {
      return;
    }
    var strings = document.getElementById("updateStrings");
    var label = strings.getFormattedString("installingPrefix", [aInstall.name]);
    var actionItem = document.getElementById("actionItem");
    actionItem.value = label;
  },

  onInstallEnded: function gInstallingPage_onInstallEnded(aInstall, aAddon) {
    if (!gUpdateWizard.shuttingDown) {
      
      AddonManagerPrivate.addStartupChange(AddonManager.STARTUP_CHANGE_CHANGED,
                                           aAddon.id);
    }

    gUpdateWizard.upgraded++;
    this.startNextInstall();
  },

  onInstallFailed: function gInstallingPage_onInstallFailed(aInstall) {
    this._errors.push(aInstall);

    gUpdateWizard.upgradeFailed++;
    this.startNextInstall();
  }
};

var gInstallErrorsPage = {
  onPageShow: function gInstallErrorsPage_onPageShow()
  {
    gUpdateWizard.setButtonLabels(null, true, null, true, null, true);
    document.documentElement.getButton("finish").focus();
  },
};



var gAdminDisabledPage = {
  onPageShow: function gAdminDisabledPage_onPageShow()
  {
    gUpdateWizard.setButtonLabels(null, true, null, true,
                                  "cancelButtonText", true);
    document.documentElement.getButton("finish").focus();
  }
};



var gFinishedPage = {
  onPageShow: function gFinishedPage_onPageShow()
  {
    gUpdateWizard.setButtonLabels(null, true, null, true, null, true);
    document.documentElement.getButton("finish").focus();

    if (gUpdateWizard.shouldSuggestAutoChecking) {
      document.getElementById("finishedCheckDisabled").hidden = false;
      gUpdateWizard.shouldAutoCheck = true;
    }
    else
      document.getElementById("finishedCheckEnabled").hidden = false;

    document.documentElement.getButton("finish").focus();
  }
};



var gNoUpdatesPage = {
  onPageShow: function gNoUpdatesPage_onPageLoad(aEvent)
  {
    gUpdateWizard.setButtonLabels(null, true, null, true, null, true);
    if (gUpdateWizard.shouldSuggestAutoChecking) {
      document.getElementById("noupdatesCheckDisabled").hidden = false;
      gUpdateWizard.shouldAutoCheck = true;
    }
    else
      document.getElementById("noupdatesCheckEnabled").hidden = false;

    gUpdateWizard.checkForErrors("updateCheckErrorNotFound");
    document.documentElement.getButton("finish").focus();
  }
};
