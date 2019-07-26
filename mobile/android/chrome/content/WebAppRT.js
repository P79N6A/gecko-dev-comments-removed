


let Cc = Components.classes;
let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");

function pref(name, value) {
  return {
    name: name,
    value: value
  }
}

let WebAppRT = {
  DEFAULT_PREFS_FILENAME: "default-prefs.js",

  prefs: [
    
    pref("extensions.enabledScopes", 1),
    
    pref("extensions.autoDisableScopes", 1),
    
    pref("xpinstall.enabled", false),
    
    pref("toolkit.telemetry.prompted", 999),
    pref("toolkit.telemetry.notifiedOptOut", 999)
  ],

  init: function(aStatus, aUrl, aCallback) {
    this.deck = document.getElementById("browsers");
    this.deck.addEventListener("click", this, false, true);

    
    if (aStatus == "new") {
      this.getDefaultPrefs().forEach(this.addPref);

      
      
      let uri = Services.io.newURI(aUrl, null, null);
      Services.perms.add(uri, "native-intent", Ci.nsIPermissionManager.DENY_ACTION);
      Services.perms.add(uri, "offline-app", Ci.nsIPermissionManager.ALLOW_ACTION);
      Services.perms.add(uri, "indexedDB", Ci.nsIPermissionManager.ALLOW_ACTION);
      Services.perms.add(uri, "indexedDB-unlimited", Ci.nsIPermissionManager.ALLOW_ACTION);

      
      let blocklist = Services.prefs.getCharPref("extensions.blocklist.url");
      blocklist = blocklist.replace(/%APP_ID%/g, "webapprt-mobile@mozilla.org");
      Services.prefs.setCharPref("extensions.blocklist.url", blocklist);
    }

    this.findManifestUrlFor(aUrl, aCallback);
  },

  findManifestUrlFor: function(aUrl, aCallback) {
    let request = navigator.mozApps.mgmt.getAll();
    request.onsuccess = function() {
      let apps = request.result;
      for (let i = 0; i < apps.length; i++) {
        let app = apps[i];
        let manifest = new ManifestHelper(app.manifest, app.origin);

        
        
        
        if (app.manifestURL == aUrl) {
          BrowserApp.manifestUrl = aUrl;
          aCallback(manifest.fullLaunchPath());
          return;
        }
    
        
        if (manifest.fullLaunchPath() == aUrl) {
          BrowserApp.manifestUrl = app.manifestURL;
          aCallback(aUrl);
          return;
        }
      }

      
      aCallback(aUrl);
    };

    request.onerror = function() {
      
      aCallback(aUrl);
    };
  },

  getDefaultPrefs: function() {
    
    try {
      let defaultPrefs = [];
      try {
          defaultPrefs = this.readDefaultPrefs(FileUtils.getFile("ProfD", [this.DEFAULT_PREFS_FILENAME]));
      } catch(ex) {
          
      }
      for (let i = 0; i < defaultPrefs.length; i++) {
        this.prefs.push(defaultPrefs[i]);
      }
    } catch(ex) {
      console.log("Error reading defaultPrefs file: " + ex);
    }
    return this.prefs;
  },

  readDefaultPrefs: function webapps_readDefaultPrefs(aFile) {
    let fstream = Cc["@mozilla.org/network/file-input-stream;1"].createInstance(Ci.nsIFileInputStream);
    fstream.init(aFile, -1, 0, 0);
    let prefsString = NetUtil.readInputStreamToString(fstream, fstream.available(), {});
    return JSON.parse(prefsString);
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

  handleEvent: function(event) {
    let target = event.target;
  
    if (!(target instanceof HTMLAnchorElement) ||
        target.getAttribute("target") != "_blank") {
      return;
    }
  
    let uri = Services.io.newURI(target.href,
                                 target.ownerDocument.characterSet,
                                 null);
  
    
    Cc["@mozilla.org/uriloader/external-protocol-service;1"].
      getService(Ci.nsIExternalProtocolService).
      getProtocolHandlerInfo(uri.scheme).
      launchWithURI(uri);
  
    
    
    
    event.preventDefault();
  }
}
