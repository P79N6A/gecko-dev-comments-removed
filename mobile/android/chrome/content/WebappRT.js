


let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/PermissionsInstaller.jsm");
Cu.import("resource://gre/modules/ContactService.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Notifications", "resource://gre/modules/Notifications.jsm");

function pref(name, value) {
  return {
    name: name,
    value: value
  }
}

let WebappRT = {
  prefs: [
    
    pref("extensions.enabledScopes", 1),
    
    pref("extensions.autoDisableScopes", 1),
    
    pref("xpinstall.enabled", false),
    
    pref("toolkit.telemetry.prompted", 999),
    pref("toolkit.telemetry.notifiedOptOut", 999),
    pref("media.useAudioChannelService", true),
    pref("dom.mozTCPSocket.enabled", true),

    
    pref("dom.sysmsg.enabled", true),
  ],

  init: function(aStatus, aUrl, aCallback) {
    this.deck = document.getElementById("browsers");
    this.deck.addEventListener("click", this, false, true);

    
    if (aStatus == "new") {
      this.prefs.forEach(this.addPref);

      
      let blocklist = Services.prefs.getCharPref("extensions.blocklist.url");
      blocklist = blocklist.replace(/%APP_ID%/g, "webapprt-mobile@mozilla.org");
      Services.prefs.setCharPref("extensions.blocklist.url", blocklist);
    }

    
    
    if (aStatus == "new" || aStatus == "upgrade") {
      this.getManifestFor(aUrl, function (aManifest, aApp) {
        if (aManifest) {
          PermissionsInstaller.installPermissions(aApp, true);
        }
      });
    }

    
    Messaging.sendRequestForResult({ type: "NativeApp:IsDebuggable" }).then((response) => {
      if (response.isDebuggable) {
        this._enableRemoteDebugger(aUrl);
      }
    });

    this.findManifestUrlFor(aUrl, (function(aLaunchUrl) {
      if (aStatus == "new") {
        if (BrowserApp.manifest && BrowserApp.manifest.orientation) {
          let orientation = BrowserApp.manifest.orientation;
          if (Array.isArray(orientation)) {
            orientation = orientation.join(",");
          }
          this.addPref(pref("app.orientation.default", orientation));
        }
      }

      aCallback(aLaunchUrl);
    }).bind(this));
  },

  getManifestFor: function (aUrl, aCallback) {
    let request = navigator.mozApps.mgmt.getAll();
    request.onsuccess = function() {
      let apps = request.result;
      for (let i = 0; i < apps.length; i++) {
        let app = apps[i];
        let manifest = new ManifestHelper(app.manifest, app.origin, app.manifestURL);

        
        if (app.manifestURL == aUrl || manifest.fullLaunchPath() == aUrl) {
          aCallback(manifest, app);
          return;
        }
      }

      
      aCallback(undefined);
    };

    request.onerror = function() {
      
      aCallback(undefined);
    };
  },

  findManifestUrlFor: function(aUrl, aCallback) {
    this.getManifestFor(aUrl, function(aManifest, aApp) {
      if (!aManifest) {
        
        aCallback(aUrl);
        return;
      }

      BrowserApp.manifest = aManifest;
      BrowserApp.manifestUrl = aApp.manifestURL;

      aCallback(aManifest.fullLaunchPath());
    });
  },

  addPref: function(aPref) {
    switch (typeof aPref.value) {
      case "string":
        Services.prefs.setCharPref(aPref.name, aPref.value);
        break;
      case "boolean":
        Services.prefs.setBoolPref(aPref.name, aPref.value);
        break;
      case "number":
        Services.prefs.setIntPref(aPref.name, aPref.value);
        break;
    }
  },

  _enableRemoteDebugger: function(aUrl) {
    
    Services.prefs.setBoolPref("devtools.debugger.prompt-connection", false);

    
    let serv = Cc['@mozilla.org/network/server-socket;1'].createInstance(Ci.nsIServerSocket);
    serv.init(-1, true, -1);
    let port = serv.port;
    serv.close();
    Services.prefs.setIntPref("devtools.debugger.remote-port", port);
    
    
    Services.prefs.setCharPref("devtools.debugger.unix-domain-socket", "");

    Services.prefs.setBoolPref("devtools.debugger.remote-enabled", true);

    
    
    DOMApplicationRegistry.registryReady.then(() => {
      let name;
      let app = DOMApplicationRegistry.getAppByManifestURL(aUrl);
      if (app) {
        name = app.name;
      } else {
        name = Strings.browser.GetStringFromName("remoteNotificationGenericName");
      }

      Notifications.create({
        title: Strings.browser.formatStringFromName("remoteNotificationTitle", [name], 1),
        message: Strings.browser.formatStringFromName("remoteNotificationMessage", [port], 1),
        icon: "drawable://warning_doorhanger",
      });
    });
  },

  handleEvent: function(event) {
    let target = event.target;

    
    while (target && !(target instanceof HTMLAnchorElement)) {
      target = target.parentNode;
    }

    if (!target || target.getAttribute("target") != "_blank") {
      return;
    }

    let uri = Services.io.newURI(target.href, target.ownerDocument.characterSet, null);

    
    Cc["@mozilla.org/uriloader/external-protocol-service;1"].
      getService(Ci.nsIExternalProtocolService).
      getProtocolHandlerInfo(uri.scheme).
      launchWithURI(uri);

    
    
    
    event.preventDefault();
  }
}
