# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Mozilla Firebird about dialog.
#
# The Initial Developer of the Original Code is
# Blake Ross (blaker@netscape.com).
# Portions created by the Initial Developer are Copyright (C) 2002
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Ehsan Akhgari <ehsan.akhgari@gmail.com>
#   Margaret Leibovic <margaret.leibovic@gmail.com>
#   Robert Strong <robert.bugzilla@gmail.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the LGPL or the GPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK ***** -->


Components.utils.import("resource://gre/modules/Services.jsm");

function init(aEvent)
{
  if (aEvent.target != document)
    return;

  try {
    var distroId = Services.prefs.getCharPref("distribution.id");
    if (distroId) {
      var distroVersion = Services.prefs.getCharPref("distribution.version");
      var distroAbout = Services.prefs.getComplexValue("distribution.about",
        Components.interfaces.nsISupportsString);

      var distroField = document.getElementById("distribution");
      distroField.value = distroAbout;
      distroField.style.display = "block";

      var distroIdField = document.getElementById("distributionId");
      distroIdField.value = distroId + " - " + distroVersion;
      distroIdField.style.display = "block";
    }
  }
  catch (e) {
    
  }

  
  let version = Services.appinfo.version;
  if (/a\d+$/.test(version)) {
    let buildID = Services.appinfo.appBuildID;
    let buildDate = buildID.slice(0,4) + "-" + buildID.slice(4,6) + "-" + buildID.slice(6,8);
    document.getElementById("version").value += " (" + buildDate + ")";
  }

#ifdef MOZ_OFFICIAL_BRANDING
  
  
  let chromeRegistry = Components.classes["@mozilla.org/chrome/chrome-registry;1"].
                       getService(Components.interfaces.nsIXULChromeRegistry);
  let currentLocale = chromeRegistry.getSelectedLocale("global");
  if (currentLocale != "en-US" && currentLocale != "en-GB") {
    document.getElementById("extra-trademark").hidden = true;
  }
#endif

#ifdef MOZ_UPDATER
  gAppUpdater = new appUpdater();
#endif

  gChannelSelector.init();

#ifdef XP_MACOSX
  
  window.sizeToContent();
  window.moveTo((screen.availWidth / 2) - (window.outerWidth / 2), screen.availHeight / 5);
#endif
}

#ifdef MOZ_UPDATER
Components.utils.import("resource:
Components.utils.import("resource:
Components.utils.import("resource:

var gAppUpdater;

function onUnload(aEvent) {
  if (gAppUpdater.isChecking)
    gAppUpdater.checker.stopChecking(Components.interfaces.nsIUpdateChecker.CURRENT_CHECK);
  
  gAppUpdater.removeDownloadListener();
  gAppUpdater = null;
}


function appUpdater()
{
  this.updateDeck = document.getElementById("updateDeck");

  
  
  if (Services.wm.getMostRecentWindow("Update:Wizard")) {
    this.updateDeck.hidden = true;
    return;
  }

  XPCOMUtils.defineLazyServiceGetter(this, "aus",
                                     "@mozilla.org/updates/update-service;1",
                                     "nsIApplicationUpdateService");
  XPCOMUtils.defineLazyServiceGetter(this, "checker",
                                     "@mozilla.org/updates/update-checker;1",
                                     "nsIUpdateChecker");
  XPCOMUtils.defineLazyServiceGetter(this, "um",
                                     "@mozilla.org/updates/update-manager;1",
                                     "nsIUpdateManager");
  XPCOMUtils.defineLazyServiceGetter(this, "bs",
                                     "@mozilla.org/extensions/blocklist;1",
                                     "nsIBlocklistService");

  this.bundle = Services.strings.
                createBundle("chrome:

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

  if (this.isPending) {
    this.setupUpdateButton("update.restart." +
                           (this.isMajor ? "upgradeButton" : "applyButton"));
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
    if (this.update)
      return this.update.state == "pending";
    return this.um.activeUpdate && this.um.activeUpdate.state == "pending";
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
    try {
      return Services.prefs.getBoolPref("app.update.enabled");
    }
    catch (e) { }
    return true; 
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
        document.commandDispatcher.focusedElement.isSameNode(this.updateBtn))
      this.updateBtn.focus();
  },

  


  buttonOnCommand: function() {
    if (this.isPending) {
      
      let cancelQuit = Components.classes["@mozilla.org/supports-PRBool;1"].
                       createInstance(Components.interfaces.nsISupportsPRBool);
      Services.obs.notifyObservers(cancelQuit, "quit-application-requested", "restart");

      
      if (cancelQuit.data)
        return;

      
      if (Services.appinfo.inSafeMode) {
        let env = Components.classes["@mozilla.org/process/environment;1"].
                  getService(Components.interfaces.nsIEnvironment);
        env.set("MOZ_SAFE_MODE_RESTART", "1");
      }

      Components.classes["@mozilla.org/toolkit/app-startup;1"].
      getService(Components.interfaces.nsIAppStartup).
      quit(Components.interfaces.nsIAppStartup.eAttemptQuit |
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
    


    onProgress: function(aRequest, aPosition, aTotalSize) {
    },

    


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
      return;
    },

    


    QueryInterface: function(aIID) {
      if (!aIID.equals(Components.interfaces.nsIUpdateCheckListener) &&
          !aIID.equals(Components.interfaces.nsISupports))
        throw Components.results.NS_ERROR_NO_INTERFACE;
      return this;
    }
  },

  


  checkAddonCompatibility: function() {
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
          if (aAddon.type != "plugin" &&
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
    if (!this.bs.isAddonBlocklisted(aAddon.id, aInstall.version,
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

    this.downloadStatus = document.getElementById("downloadStatus");
    this.downloadStatus.value =
      DownloadUtils.getTransferTotal(0, this.update.selectedPatch.size);
    this.selectPanel("downloading");
    this.aus.addDownloadListener(this);
  },

  removeDownloadListener: function() {
    this.aus.removeDownloadListener(this);
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
      this.selectPanel("updateButtonBox");
      this.setupUpdateButton("update.restart." +
                             (this.isMajor ? "upgradeButton" : "applyButton"));
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

var gChannelSelector = {
  validChannels: { release: 1, beta: 1, aurora: 1 },
  
  init: function() {
    try {
      this.channelValue = Services.prefs.getCharPref("app.update.desiredChannel");
    } catch (e) {
      let defaults = Services.prefs.getDefaultBranch("");
      this.channelValue = defaults.getCharPref("app.update.channel");
    }

    
    if (this.channelValue in this.validChannels) {
      document.getElementById("currentChannelText").hidden = false;
      this.setChannelLabel(this.channelValue);
      this.setChannelMenuitem(this.channelValue);
    }
  },

  selectChannel: function(aSelectedItem) {
    document.getElementById("channelDescriptionDeck").selectedPanel =
      document.getElementById(aSelectedItem.value + "Description");
    document.getElementById("channelMenulist").setAttribute("aria-describedby",
      aSelectedItem.value + "Description");
  },

  cancel: function() {
    this.setChannelMenuitem(this.channelValue);
    this.hide();
  },

  apply: function() {
    this.channelValue = document.getElementById("channelMenulist").selectedItem.value;
    this.setChannelLabel(this.channelValue);

    
    Services.prefs.setCharPref("app.update.desiredChannel", this.channelValue);

    
    
    gAppUpdater.isChecking = true;
    gAppUpdater.checker.checkForUpdates(gAppUpdater.updateCheckListener, true);

    this.hide();
  },

  show: function() {
    document.getElementById("contentDeck").selectedPanel =
      document.getElementById("channelSelector");
  },

  hide: function() {
    document.getElementById("contentDeck").selectedPanel =
      document.getElementById("detailsBox");  
  },

  setChannelLabel: function(aValue) {
    let channelLabel = document.getElementById("currentChannel");
    channelLabel.value = document.getElementById(aValue + "Menuitem").label;
  },

  setChannelMenuitem: function(aValue) {
    document.getElementById("channelMenulist").selectedItem =
      document.getElementById(aValue + "Menuitem");
  }
}
