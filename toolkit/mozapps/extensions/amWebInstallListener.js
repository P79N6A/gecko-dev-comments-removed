










"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/AddonManager.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const URI_XPINSTALL_DIALOG = "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul";


const READY_STATES = [
  AddonManager.STATE_AVAILABLE,
  AddonManager.STATE_DOWNLOAD_FAILED,
  AddonManager.STATE_INSTALL_FAILED,
  AddonManager.STATE_CANCELLED
];

["LOG", "WARN", "ERROR"].forEach(function(aName) {
  this.__defineGetter__(aName, function logFuncGetter() {
    Components.utils.import("resource://gre/modules/AddonLogging.jsm");

    LogManager.getLogger("addons.weblistener", this);
    return this[aName];
  });
}, this);

function notifyObservers(aTopic, aWindow, aUri, aInstalls) {
  let info = {
    originatingWindow: aWindow,
    originatingURI: aUri,
    installs: aInstalls,

    QueryInterface: XPCOMUtils.generateQI([Ci.amIWebInstallInfo])
  };
  Services.obs.notifyObservers(info, aTopic, null);
}












function Installer(aWindow, aUrl, aInstalls) {
  this.window = aWindow;
  this.url = aUrl;
  this.downloads = aInstalls;
  this.installed = [];

  notifyObservers("addon-install-started", aWindow, aUrl, aInstalls);

  aInstalls.forEach(function(aInstall) {
    aInstall.addListener(this);

    
    if (READY_STATES.indexOf(aInstall.state) != -1)
      aInstall.install();
  }, this);

  this.checkAllDownloaded();
}

Installer.prototype = {
  window: null,
  downloads: null,
  installed: null,
  isDownloading: true,

  


  checkAllDownloaded: function Installer_checkAllDownloaded() {
    
    
    if (!this.isDownloading)
      return;

    var failed = [];
    var installs = [];

    for (let install of this.downloads) {
      switch (install.state) {
      case AddonManager.STATE_AVAILABLE:
      case AddonManager.STATE_DOWNLOADING:
        
        
        return;
      case AddonManager.STATE_DOWNLOAD_FAILED:
        failed.push(install);
        break;
      case AddonManager.STATE_DOWNLOADED:
        
        if (install.addon.appDisabled)
          failed.push(install);
        else
          installs.push(install);

        if (install.linkedInstalls) {
          install.linkedInstalls.forEach(function(aInstall) {
            aInstall.addListener(this);
            
            if (aInstall.addon.appDisabled)
              failed.push(aInstall);
            else
              installs.push(aInstall);
          }, this);
        }
        break;
      case AddonManager.STATE_CANCELLED:
        
        break;
      default:
        WARN("Download of " + install.sourceURI.spec + " in unexpected state " +
             install.state);
      }
    }

    this.isDownloading = false;
    this.downloads = installs;

    if (failed.length > 0) {
      
      
      failed.forEach(function(aInstall) {
        if (aInstall.state == AddonManager.STATE_DOWNLOADED) {
          aInstall.removeListener(this);
          aInstall.cancel();
        }
      }, this);
      notifyObservers("addon-install-failed", this.window, this.url, failed);
    }

    
    if (this.downloads.length == 0)
      return;

    
    
    if ("@mozilla.org/addons/web-install-prompt;1" in Cc) {
      try {
        let prompt = Cc["@mozilla.org/addons/web-install-prompt;1"].
                     getService(Ci.amIWebInstallPrompt);
        prompt.confirm(this.window, this.url, this.downloads, this.downloads.length);
        return;
      }
      catch (e) {}
    }

    let args = {};
    args.url = this.url;
    args.installs = this.downloads;
    args.wrappedJSObject = args;

    try {
      Cc["@mozilla.org/base/telemetry;1"].
            getService(Ci.nsITelemetry).
            getHistogramById("SECURITY_UI").
            add(Ci.nsISecurityUITelemetry.WARNING_CONFIRM_ADDON_INSTALL);
      Services.ww.openWindow(this.window, URI_XPINSTALL_DIALOG,
                             null, "chrome,modal,centerscreen", args);
    } catch (e) {
      this.downloads.forEach(function(aInstall) {
        aInstall.removeListener(this);
        
        
        aInstall.cancel();
      }, this);
      notifyObservers("addon-install-cancelled", this.window, this.url,
                      this.downloads);
    }
  },

  


  checkAllInstalled: function Installer_checkAllInstalled() {
    var failed = [];

    for (let install of this.downloads) {
      switch(install.state) {
      case AddonManager.STATE_DOWNLOADED:
      case AddonManager.STATE_INSTALLING:
        
        
        return;
      case AddonManager.STATE_INSTALL_FAILED:
        failed.push(install);
        break;
      }
    }

    this.downloads = null;

    if (failed.length > 0)
      notifyObservers("addon-install-failed", this.window, this.url, failed);

    if (this.installed.length > 0)
      notifyObservers("addon-install-complete", this.window, this.url, this.installed);
    this.installed = null;
  },

  onDownloadCancelled: function Installer_onDownloadCancelled(aInstall) {
    aInstall.removeListener(this);
    this.checkAllDownloaded();
  },

  onDownloadFailed: function Installer_onDownloadFailed(aInstall) {
    aInstall.removeListener(this);
    this.checkAllDownloaded();
  },

  onDownloadEnded: function Installer_onDownloadEnded(aInstall) {
    this.checkAllDownloaded();
    return false;
  },

  onInstallCancelled: function Installer_onInstallCancelled(aInstall) {
    aInstall.removeListener(this);
    this.checkAllInstalled();
  },

  onInstallFailed: function Installer_onInstallFailed(aInstall) {
    aInstall.removeListener(this);
    this.checkAllInstalled();
  },

  onInstallEnded: function Installer_onInstallEnded(aInstall) {
    aInstall.removeListener(this);
    this.installed.push(aInstall);

    
    if (aInstall.addon.type == "theme" &&
        aInstall.addon.userDisabled == true &&
        aInstall.addon.appDisabled == false) {
      aInstall.addon.userDisabled = false;
    }

    this.checkAllInstalled();
  }
};

function extWebInstallListener() {
}

extWebInstallListener.prototype = {
  


  onWebInstallDisabled: function extWebInstallListener_onWebInstallDisabled(aWindow, aUri, aInstalls) {
    let info = {
      originatingWindow: aWindow,
      originatingURI: aUri,
      installs: aInstalls,

      QueryInterface: XPCOMUtils.generateQI([Ci.amIWebInstallInfo])
    };
    Services.obs.notifyObservers(info, "addon-install-disabled", null);
  },

  


  onWebInstallBlocked: function extWebInstallListener_onWebInstallBlocked(aWindow, aUri, aInstalls) {
    let info = {
      originatingWindow: aWindow,
      originatingURI: aUri,
      installs: aInstalls,

      install: function onWebInstallBlocked_install() {
        new Installer(this.originatingWindow, this.originatingURI, this.installs);
      },

      QueryInterface: XPCOMUtils.generateQI([Ci.amIWebInstallInfo])
    };
    Services.obs.notifyObservers(info, "addon-install-blocked", null);

    return false;
  },

  


  onWebInstallRequested: function extWebInstallListener_onWebInstallRequested(aWindow, aUri, aInstalls) {
    new Installer(aWindow, aUri, aInstalls);

    
    return false;
  },

  classDescription: "XPI Install Handler",
  contractID: "@mozilla.org/addons/web-install-listener;1",
  classID: Components.ID("{0f38e086-89a3-40a5-8ffc-9b694de1d04a}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.amIWebInstallListener])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([extWebInstallListener]);
