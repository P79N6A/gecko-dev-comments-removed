










"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/AddonManager.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PromptUtils", "resource://gre/modules/SharedPromptUtils.jsm");

const URI_XPINSTALL_DIALOG = "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul";


const READY_STATES = [
  AddonManager.STATE_AVAILABLE,
  AddonManager.STATE_DOWNLOAD_FAILED,
  AddonManager.STATE_INSTALL_FAILED,
  AddonManager.STATE_CANCELLED
];

Cu.import("resource://gre/modules/Log.jsm");
const LOGGER_ID = "addons.weblistener";



let logger = Log.repository.getLogger(LOGGER_ID);

function notifyObservers(aTopic, aBrowser, aUri, aInstalls) {
  let info = {
    browser: aBrowser,
    originatingURI: aUri,
    installs: aInstalls,

    QueryInterface: XPCOMUtils.generateQI([Ci.amIWebInstallInfo])
  };
  Services.obs.notifyObservers(info, aTopic, null);
}












function Installer(aBrowser, aUrl, aInstalls) {
  this.browser = aBrowser;
  this.url = aUrl;
  this.downloads = aInstalls;
  this.installed = [];

  notifyObservers("addon-install-started", aBrowser, aUrl, aInstalls);

  aInstalls.forEach(function(aInstall) {
    aInstall.addListener(this);

    
    if (READY_STATES.indexOf(aInstall.state) != -1)
      aInstall.install();
  }, this);

  this.checkAllDownloaded();
}

Installer.prototype = {
  browser: null,
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
        logger.warn("Download of " + install.sourceURI.spec + " in unexpected state " +
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
      notifyObservers("addon-install-failed", this.browser, this.url, failed);
    }

    
    if (this.downloads.length == 0)
      return;

    
    
    if ("@mozilla.org/addons/web-install-prompt;1" in Cc) {
      try {
        let prompt = Cc["@mozilla.org/addons/web-install-prompt;1"].
                     getService(Ci.amIWebInstallPrompt);
        prompt.confirm(this.browser, this.url, this.downloads, this.downloads.length);
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
      let parentWindow = null;
      if (this.browser) {
        parentWindow = this.browser.ownerDocument.defaultView;
        PromptUtils.fireDialogEvent(parentWindow, "DOMWillOpenModalDialog", this.browser);
      }
      Services.ww.openWindow(parentWindow, URI_XPINSTALL_DIALOG,
                             null, "chrome,modal,centerscreen", args);
    } catch (e) {
      logger.warn("Exception showing install confirmation dialog", e);
      this.downloads.forEach(function(aInstall) {
        aInstall.removeListener(this);
        
        
        aInstall.cancel();
      }, this);
      notifyObservers("addon-install-cancelled", this.browser, this.url,
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
      notifyObservers("addon-install-failed", this.browser, this.url, failed);

    if (this.installed.length > 0)
      notifyObservers("addon-install-complete", this.browser, this.url, this.installed);
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
  


  onWebInstallDisabled: function extWebInstallListener_onWebInstallDisabled(aBrowser, aUri, aInstalls) {
    let info = {
      browser: aBrowser,
      originatingURI: aUri,
      installs: aInstalls,

      QueryInterface: XPCOMUtils.generateQI([Ci.amIWebInstallInfo])
    };
    Services.obs.notifyObservers(info, "addon-install-disabled", null);
  },

  


  onWebInstallBlocked: function extWebInstallListener_onWebInstallBlocked(aBrowser, aUri, aInstalls) {
    let info = {
      browser: aBrowser,
      originatingURI: aUri,
      installs: aInstalls,

      install: function onWebInstallBlocked_install() {
        new Installer(this.browser, this.originatingURI, this.installs);
      },

      QueryInterface: XPCOMUtils.generateQI([Ci.amIWebInstallInfo])
    };
    Services.obs.notifyObservers(info, "addon-install-blocked", null);

    return false;
  },

  


  onWebInstallRequested: function extWebInstallListener_onWebInstallRequested(aBrowser, aUri, aInstalls) {
    new Installer(aBrowser, aUri, aInstalls);

    
    return false;
  },

  classDescription: "XPI Install Handler",
  contractID: "@mozilla.org/addons/web-install-listener;1",
  classID: Components.ID("{0f38e086-89a3-40a5-8ffc-9b694de1d04a}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.amIWebInstallListener])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([extWebInstallListener]);
