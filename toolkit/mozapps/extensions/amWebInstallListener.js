













































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/AddonManager.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");







function WARN(str) {
  dump("*** addons.weblistener: " + str + "\n");
}












function Installer(window, url, installs) {
  this.window = window;
  this.url = url;
  this.downloads = installs;
  this.installs = [];

  this.bundle = Cc["@mozilla.org/intl/stringbundle;1"].
                getService(Ci.nsIStringBundleService).
                createBundle("chrome://mozapps/locale/extensions/extensions.properties");

  this.count = installs.length;
  installs.forEach(function(install) {
    install.addListener(this);

    
    if (install.state == AddonManager.STATE_DOWNLOADED)
      this.onDownloadEnded(install);
    else
      install.install();
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

    let args = {};
    args.url = this.url;
    args.installs = this.installs;
    args.wrappedJSObject = args;

    Services.ww.openWindow(this.window, "chrome://mozapps/content/xpinstall/xpinstallConfirm.xul",
                           null, "chrome,modal,centerscreen", args);
  },

  onDownloadCancelled: function(install) {
    install.removeListener(this);

    this.checkAllDownloaded();
  },

  onDownloadFailed: function(install, error) {
    install.removeListener(this);

    
    Services.prompt.alert(this.window, "Download Failed", "The download of " + install.sourceURL + " failed: " + error);
    this.checkAllDownloaded();
  },

  onDownloadEnded: function(install) {
    install.removeListener(this);

    if (install.addon.appDisabled) {
      
      install.cancel();

      let title = null;
      let text = null;

      let problems = "";
      if (!install.addon.isCompatible)
        problems += "incompatible, ";
      if (!install.addon.providesUpdatesSecurely)
        problems += "insecure updates, ";
      if (install.addon.blocklistState == Ci.nsIBlocklistService.STATE_BLOCKED) {
        problems += "blocklisted, ";
        title = bundle.GetStringFromName("blocklistedInstallTitle2");
        text = this.bundle.formatStringFromName("blocklistedInstallMsg2",
                                                [install.addon.name], 1);
      }
      problems = problems.substring(0, problems.length - 2);
      WARN("Not installing " + install.addon.id + " because of the following: " + problems);

      title = this.bundle.GetStringFromName("incompatibleTitle2", 1);
      text = this.bundle.formatStringFromName("incompatibleMessage",
                                              [install.addon.name,
                                               install.addon.version,
                                               Services.appinfo.name,
                                               Services.appinfo.version], 4);
      Services.prompt.alert(this.window, title, text);
    }
    else {
      this.installs.push(install);
    }

    this.checkAllDownloaded();
    return false;
  },
};

function extWebInstallListener() {
}

extWebInstallListener.prototype = {
  


  onWebInstallBlocked: function(window, uri, installs) {
    let info = {
      originatingWindow: window,
      originatingURI: uri,
      installs: installs,

      install: function() {
        dump("Start installs\n");
        new Installer(this.originatingWindow, this.originatingURI, this.installs);
      },

      QueryInterface: XPCOMUtils.generateQI([Ci.amIWebInstallInfo])
    };
    Services.obs.notifyObservers(info, "addon-install-blocked", null);

    return false;
  },

  


  onWebInstallRequested: function(window, uri, installs) {
    new Installer(window, uri, installs);

    
    return false;
  },

  classDescription: "XPI Install Handler",
  contractID: "@mozilla.org/addons/web-install-listener;1",
  classID: Components.ID("{0f38e086-89a3-40a5-8ffc-9b694de1d04a}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.amIWebInstallListener])
};

function NSGetModule(compMgr, fileSpec)
  XPCOMUtils.generateModule([extWebInstallListener]);
