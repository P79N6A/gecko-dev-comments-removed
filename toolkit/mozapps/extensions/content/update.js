







"use strict";

const PREF_UPDATE_EXTENSIONS_ENABLED            = "extensions.update.enabled";
const PREF_XPINSTALL_ENABLED                    = "xpinstall.enabled";
const PREF_EM_HOTFIX_ID                         = "extensions.hotfix.id";

Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/AddonManager.jsm");
Components.utils.import("resource://gre/modules/AddonRepository.jsm");


var gInteruptable = true;
var gPendingClose = false;


var gUpdateWizard = {
  
  
  
  addons: [],
  
  
  inactiveAddonIDs: [],
  
  addonsToUpdate: [],
  shouldSuggestAutoChecking: false,
  shouldAutoCheck: false,
  xpinstallEnabled: true,
  xpinstallLocked: false,

  init: function gUpdateWizard_init()
  {
    this.inactiveAddonIDs = window.arguments[0];

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
    if (!gInteruptable) {
      gPendingClose = true;
      this._setUpButton("back", null, true);
      this._setUpButton("next", null, true);
      this._setUpButton("cancel", null, true);
      return false;
    }

    if (gInstallingPage.installing) {
      gInstallingPage.cancelInstalls();
      return false;
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

var gVersionInfoPage = {
  _completeCount: 0,
  _totalCount: 0,
  onPageShow: function gVersionInfoPage_onPageShow()
  {
    gUpdateWizard.setButtonLabels(null, true,
                                  "nextButtonText", true,
                                  "cancelButtonText", false);

    try {
      var hotfixID = Services.prefs.getCharPref(PREF_EM_HOTFIX_ID);
    }
    catch (e) { }

    
    AddonManager.getAllAddons(function gVersionInfoPage_getAllAddons(aAddons) {
      gUpdateWizard.addons = aAddons.filter(function gVersionInfoPage_filterAddons(a) {
        return a.type != "plugin" && a.id != hotfixID;
      });

      gVersionInfoPage._totalCount = gUpdateWizard.addons.length;

      
      
      let ids = [addon.id for each (addon in gUpdateWizard.addons)];

      gInteruptable = false;
      AddonRepository.repopulateCache(ids, function gVersionInfoPage_repolulateCache() {
        AddonManagerPrivate.updateAddonRepositoryData(function gVersionInfoPage_updateAddonRepoData() {
          gInteruptable = true;
          if (gPendingClose) {
            window.close();
            return;
          }

          for (let addon of gUpdateWizard.addons)
            addon.findUpdates(gVersionInfoPage, AddonManager.UPDATE_WHEN_NEW_APP_INSTALLED);
        });
      });
    });
  },

  onAllUpdatesFinished: function gVersionInfoPage_onAllUpdatesFinished() {
    
    
    gUpdateWizard.addons = gUpdateWizard.addons.filter(function onAllUpdatesFinished_filterAddons(a) {
      return a.appDisabled && gUpdateWizard.inactiveAddonIDs.indexOf(a.id) < 0;
    });

    if (gUpdateWizard.addons.length > 0) {
      
      document.documentElement.currentPage = document.getElementById("mismatch");
    }
    else {
      
      
      
      
      setTimeout(close, 0);
    }
  },

  
  
  onUpdateFinished: function gVersionInfoPage_onUpdateFinished(aAddon, status) {
    
    if (aAddon.active)
      AddonManagerPrivate.removeStartupChange("disabled", aAddon.id);

    if (status != AddonManager.UPDATE_STATUS_NO_ERROR)
      gUpdateWizard.errorItems.push(aAddon);

    ++this._completeCount;

    
    var updateStrings = document.getElementById("updateStrings");
    var status = document.getElementById("versioninfo.status");
    var statusString = updateStrings.getFormattedString("statusPrefix", [aAddon.name]);
    status.setAttribute("value", statusString);

    
    var progress = document.getElementById("versioninfo.progress");
    progress.mode = "normal";
    progress.value = Math.ceil((this._completeCount / this._totalCount) * 100);

    if (this._completeCount == this._totalCount)
      this.onAllUpdatesFinished();
  },
};

var gMismatchPage = {
  onPageShow: function gMismatchPage_onPageShow()
  {
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
    if (!gUpdateWizard.xpinstallEnabled && gUpdateWizard.xpinstallLocked) {
      document.documentElement.currentPage = document.getElementById("adminDisabled");
      return;
    }

    gUpdateWizard.setButtonLabels(null, true,
                                  "nextButtonText", true,
                                  "cancelButtonText", false);
    document.documentElement.getButton("next").focus();

    gUpdateWizard.errorItems = [];

    this._totalCount = gUpdateWizard.addons.length;
    for (let addon of gUpdateWizard.addons)
      addon.findUpdates(this, AddonManager.UPDATE_WHEN_NEW_APP_INSTALLED);
  },

  onAllUpdatesFinished: function gUpdatePage_onAllUpdatesFinished() {
    var nextPage = document.getElementById("noupdates");
    if (gUpdateWizard.addonsToUpdate.length > 0)
      nextPage = document.getElementById("found");
    document.documentElement.currentPage = nextPage;
  },

  
  
  onUpdateAvailable: function gUpdatePage_onUpdateAvailable(aAddon, aInstall) {
    gUpdateWizard.addonsToUpdate.push(aInstall);
  },

  onUpdateFinished: function gUpdatePage_onUpdateFinished(aAddon, status) {
    if (status != AddonManager.UPDATE_STATUS_NO_ERROR)
      gUpdateWizard.errorItems.push(aAddon);

    ++this._completeCount;

    
    var updateStrings = document.getElementById("updateStrings");
    var status = document.getElementById("checking.status");
    var statusString = updateStrings.getFormattedString("statusPrefix", [aAddon.name]);
    status.setAttribute("value", statusString);

    var progress = document.getElementById("checking.progress");
    progress.value = Math.ceil((this._completeCount / this._totalCount) * 100);

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

  onPageShow: function gInstallingPage_onPageShow()
  {
    gUpdateWizard.setButtonLabels(null, true,
                                  "nextButtonText", true,
                                  null, true);
    this._errors = [];

    var foundUpdates = document.getElementById("found.updates");
    var updates = foundUpdates.getElementsByTagName("listitem");
    for (let update of updates) {
      if (!update.checked)
        continue;
      this._installs.push(update.install);
    }

    this._strings = document.getElementById("updateStrings");
    this._installing = true;
    this.startNextInstall();
  },

  startNextInstall: function gInstallingPage_startNextInstall() {
    if (this._currentInstall >= 0) {
      this._installs[this._currentInstall].removeListener(this);
    }

    this._currentInstall++;

    if (this._installs.length == this._currentInstall) {
      this._installing = false;
      var nextPage = this._errors.length > 0 ? "installerrors" : "finished";
      document.getElementById("installing").setAttribute("next", nextPage);
      document.documentElement.advance();
      return;
    }

    this._installs[this._currentInstall].addListener(this);
    this._installs[this._currentInstall].install();
  },

  cancelInstalls: function gInstallingPage_cancelInstalls() {
    this._installs[this._currentInstall].removeListener(this);
    this._installs[this._currentInstall].cancel();
  },

  
  
  onDownloadStarted: function gInstallingPage_onDownloadStarted(aInstall) {
    var strings = document.getElementById("updateStrings");
    var label = strings.getFormattedString("downloadingPrefix", [aInstall.name]);
    var actionItem = document.getElementById("actionItem");
    actionItem.value = label;
  },

  onDownloadProgress: function gInstallingPage_onDownloadProgress(aInstall) {
    var downloadProgress = document.getElementById("downloadProgress");
    downloadProgress.value = Math.ceil(100 * aInstall.progress / aInstall.maxProgress);
  },

  onDownloadEnded: function gInstallingPage_onDownloadEnded(aInstall) {
  },

  onDownloadFailed: function gInstallingPage_onDownloadFailed(aInstall) {
    this._errors.push(aInstall);

    this.startNextInstall();
  },

  onInstallStarted: function gInstallingPage_onInstallStarted(aInstall) {
    var strings = document.getElementById("updateStrings");
    var label = strings.getFormattedString("installingPrefix", [aInstall.name]);
    var actionItem = document.getElementById("actionItem");
    actionItem.value = label;
  },

  onInstallEnded: function gInstallingPage_onInstallEnded(aInstall, aAddon) {
    
    AddonManagerPrivate.addStartupChange(AddonManager.STARTUP_CHANGE_CHANGED,
                                         aAddon.id);

    this.startNextInstall();
  },

  onInstallFailed: function gInstallingPage_onInstallFailed(aInstall) {
    this._errors.push(aInstall);

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
