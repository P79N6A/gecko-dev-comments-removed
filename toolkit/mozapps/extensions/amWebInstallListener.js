













































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/AddonManager.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

["LOG", "WARN", "ERROR"].forEach(function(aName) {
  this.__defineGetter__(aName, function() {
    Components.utils.import("resource://gre/modules/AddonLogging.jsm");

    LogManager.getLogger("addons.weblistener", this);
    return this[aName];
  });
}, this);












function Installer(aWindow, aUrl, aInstalls) {
  this.window = aWindow;
  this.url = aUrl;
  this.downloads = aInstalls;
  this.installs = [];

  this.bundle = Cc["@mozilla.org/intl/stringbundle;1"].
                getService(Ci.nsIStringBundleService).
                createBundle("chrome://mozapps/locale/extensions/extensions.properties");

  this.count = aInstalls.length;
  aInstalls.forEach(function(aInstall) {
    aInstall.addListener(this);

    
    if (aInstall.state == AddonManager.STATE_DOWNLOADED)
      this.onDownloadEnded(aInstall);
    else if (aInstall.state == AddonManager.STATE_DOWNLOAD_FAILED)
      this.onDownloadFailed(aInstall);
    else
      aInstall.install();
  }, this);
}

Installer.prototype = {
  window: null,
  downloads: null,
  installs: null,
  count: null,

  


  checkAllDownloaded: function() {
    if (--this.count > 0)
      return;

    
    if (this.installs.length == 0)
      return;

    if ("@mozilla.org/addons/web-install-prompt;1" in Cc) {
      try {
        let prompt = Cc["@mozilla.org/addons/web-install-prompt;1"].
                     getService(Ci.amIWebInstallPrompt);
        prompt.confirm(this.window, this.url, this.installs, this.installs.length);
        return;
      }
      catch (e) {}
    }

    let args = {};
    args.url = this.url;
    args.installs = this.installs;
    args.wrappedJSObject = args;

    Services.ww.openWindow(this.window, "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul",
                           null, "chrome,modal,centerscreen", args);
  },

  onDownloadCancelled: function(aInstall) {
    aInstall.removeListener(this);

    this.checkAllDownloaded();
  },

  onDownloadFailed: function(aInstall) {
    aInstall.removeListener(this);

    
    Services.prompt.alert(this.window, "Download Failed", "The download of " +
                          aInstall.sourceURL + " failed: " + aInstall.error);
    this.checkAllDownloaded();
  },

  onDownloadEnded: function(aInstall) {
    aInstall.removeListener(this);

    if (aInstall.addon.appDisabled) {
      
      aInstall.cancel();

      let title = null;
      let text = null;

      let problems = "";
      if (!aInstall.addon.isCompatible)
        problems += "incompatible, ";
      if (!aInstall.addon.providesUpdatesSecurely)
        problems += "insecure updates, ";
      if (aInstall.addon.blocklistState == Ci.nsIBlocklistService.STATE_BLOCKED) {
        problems += "blocklisted, ";
        title = bundle.GetStringFromName("blocklistedInstallTitle2");
        text = this.bundle.formatStringFromName("blocklistedInstallMsg2",
                                                [install.addon.name], 1);
      }
      problems = problems.substring(0, problems.length - 2);
      WARN("Not installing " + aInstall.addon.id + " because of the following: " + problems);

      title = this.bundle.GetStringFromName("incompatibleTitle2", 1);
      text = this.bundle.formatStringFromName("incompatibleMessage2",
                                              [aInstall.addon.name,
                                               aInstall.addon.version,
                                               Services.appinfo.name,
                                               Services.appinfo.version], 4);
      Services.prompt.alert(this.window, title, text);
    }
    else {
      this.installs.push(aInstall);
    }

    this.checkAllDownloaded();
    return false;
  },
};

function extWebInstallListener() {
}

extWebInstallListener.prototype = {
  


  onWebInstallBlocked: function(aWindow, aUri, aInstalls) {
    let info = {
      originatingWindow: aWindow,
      originatingURI: aUri,
      installs: aInstalls,

      install: function() {
        new Installer(this.originatingWindow, this.originatingURI, this.installs);
      },

      QueryInterface: XPCOMUtils.generateQI([Ci.amIWebInstallInfo])
    };
    Services.obs.notifyObservers(info, "addon-install-blocked", null);

    return false;
  },

  


  onWebInstallRequested: function(aWindow, aUri, aInstalls) {
    new Installer(aWindow, aUri, aInstalls);

    
    return false;
  },

  classDescription: "XPI Install Handler",
  contractID: "@mozilla.org/addons/web-install-listener;1",
  classID: Components.ID("{0f38e086-89a3-40a5-8ffc-9b694de1d04a}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.amIWebInstallListener])
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([extWebInstallListener]);
