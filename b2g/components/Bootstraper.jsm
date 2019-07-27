



"use strict";

this.EXPORTED_SYMBOLS = ["Bootstraper"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const CC = Components.Constructor;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");

function debug(aMsg) {
  
}





this.Bootstraper = {
  _manifestURL: null,
  _startupURL: null,

  bailout: function(aMsg) {
    dump("************************************************************\n");
    dump("* /!\\ " + aMsg + "\n");
    dump("************************************************************\n");
    let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"]
                       .getService(Ci.nsIAppStartup);
    appStartup.quit(appStartup.eForceQuit);
  },

  installSystemApp: function(aManifest) {
    
    let base = Services.io.newURI(this._manifestURL, null, null);
    let origin = base.prePath;
    let helper = new ManifestHelper(aManifest, origin, this._manifestURL);
    this._startupURL = helper.fullLaunchPath();

    return new Promise((aResolve, aReject) => {
      debug("Origin is " + origin);
      let appData = {
        app: {
          installOrigin: origin,
          origin: origin,
          manifest: aManifest,
          manifestURL: this._manifestURL,
          manifestHash: AppsUtils.computeHash(JSON.stringify(aManifest)),
          appStatus: Ci.nsIPrincipal.APP_STATUS_CERTIFIED
        },
        appId: 1,
        isBrowser: false,
        isPackage: false
      };

      DOMApplicationRegistry.confirmInstall(appData, null, aResolve);
    });
  },

  


  loadManifest: function() {
    return new Promise((aResolve, aReject) => {
      debug("Loading manifest " + this._manifestURL);

      let xhr =  Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                 .createInstance(Ci.nsIXMLHttpRequest);
      xhr.mozBackgroundRequest = true;
      xhr.open("GET", this._manifestURL);
      xhr.responseType = "json";
      xhr.addEventListener("load", () => {
        if (xhr.status >= 200 && xhr.status < 400) {
          debug("Success loading " + this._manifestURL);
          aResolve(xhr.response);
        } else {
          aReject("Error loading " + this._manifestURL);
        }
      });
      xhr.addEventListener("error", () => {
        aReject("Error loading " + this._manifestURL);
      });
      xhr.send(null);
    });
  },

  configure: function() {
    debug("Setting startup prefs... " + this._startupURL);
    Services.prefs.setCharPref("b2g.system_manifest_url", this._manifestURL);
    Services.prefs.setCharPref("b2g.system_startup_url", this._startupURL);
    return Promise.resolve();
  },

  



  uninstallPreviousSystemApp: function() {
    let oldManifestURL;
    try{
      oldManifestURL = Services.prefs.getCharPref("b2g.system_manifest_url");
    } catch(e) {
      
      return Promise.resolve();
    }

    let id = DOMApplicationRegistry.getAppLocalIdByManifestURL(oldManifestURL);
    if (id == Ci.nsIScriptSecurityManager.NO_APP_ID) {
      return Promise.resolve();
    }
    debug("Uninstalling " + oldManifestURL);
    return DOMApplicationRegistry.uninstall(oldManifestURL);
  },

  


  ensureSystemAppInstall: function(aManifestURL) {
    this._manifestURL = aManifestURL;
    debug("Installing app from " + this._manifestURL);

    
    
    try {
      if (Services.prefs.getCharPref("b2g.system_manifest_url") == this._manifestURL) {
        debug("Already configured for " + this._manifestURL);
        return Promise.resolve();
      }
    } catch(e) { }

    return new Promise((aResolve, aReject) => {
      DOMApplicationRegistry.registryReady
          .then(this.uninstallPreviousSystemApp.bind(this))
          .then(this.loadManifest.bind(this))
          .then(this.installSystemApp.bind(this))
          .then(this.configure.bind(this))
          .then(aResolve)
          .catch(aReject);
    });
  }
};
