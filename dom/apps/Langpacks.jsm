



"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");

this.EXPORTED_SYMBOLS = ["Langpacks"];

let debug = Services.prefs.getBoolPref("dom.mozApps.debug")
  ? (aMsg) => {
      dump("-*-*- Langpacks: " + aMsg + "\n");
    }
  : (aMsg) => {};



















this.Langpacks = {

  _data: {},
  _broadcaster: null,
  _appIdFromManifestURL: null,

  init: function() {
    ppmm.addMessageListener("Webapps:GetLocalizationResource", this);
  },

  registerRegistryFunctions: function(aBroadcaster, aIdGetter) {
    this._broadcaster = aBroadcaster;
    this._appIdFromManifestURL = aIdGetter;
  },

  receiveMessage: function(aMessage) {
    let data = aMessage.data;
    let mm = aMessage.target;
    switch (aMessage.name) {
      case "Webapps:GetLocalizationResource":
        this.getLocalizationResource(data, mm);
        break;
      default:
        debug("Unexpected message: " + aMessage.name);
    }
  },

  getAdditionalLanguages: function(aManifestURL) {
    debug("getAdditionalLanguages " + aManifestURL);
    let res = { langs: {} };
    let langs = res.langs;
    if (this._data[aManifestURL]) {
      res.appId = this._data[aManifestURL].appId;
      for (let lang in this._data[aManifestURL].langs) {
        if (!langs[lang]) {
          langs[lang] = [];
        }
        let current = this._data[aManifestURL].langs[lang];
        langs[lang].push({
          version: current.version,
          name: current.name,
          target: current.target
        });
      }
    }
    debug("Languages found: " + uneval(res));
    return res;
  },

  sendAppUpdate: function(aManifestURL) {
    debug("sendAppUpdate " + aManifestURL);
    if (!this._broadcaster) {
      debug("No broadcaster!");
      return;
    }

    let res = this.getAdditionalLanguages(aManifestURL);
    let message = {
      id: res.appId,
      app: {
        additionalLanguages: res.langs
      }
    }
    this._broadcaster("Webapps:UpdateState", message);
  },

  getLocalizationResource: function(aData, aMm) {
    debug("getLocalizationResource " + uneval(aData));

    function sendError(aMsg, aCode) {
      debug(aMsg);
      aMm.sendAsyncMessage("Webapps:GetLocalizationResource:Return",
        { requestID: aData.requestID, oid: aData.oid, error: aCode });
    }

    
    if (!this._data[aData.manifestURL]) {
      return sendError("No langpack for this app.", "NoLangpack");
    }

    
    if (!this._data[aData.manifestURL].langs[aData.lang]) {
      return sendError("No language " + aData.lang + " for this app.",
                       "UnavailableLanguage");
    }

    
    let item = this._data[aData.manifestURL].langs[aData.lang];
    if (item.target != aData.version) {
      return sendError("No version " + aData.version + " for this app.",
                       "UnavailableVersion");
    }

    
    if (isAbsoluteURI(aData.path)) {
      return sendError("url can't be absolute.", "BadUrl");
    }

    let href = item.url + aData.path;
    debug("Will load " + href);

    let xhr =  Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                 .createInstance(Ci.nsIXMLHttpRequest);
    xhr.mozBackgroundRequest = true;
    xhr.open("GET", href);

    
    
    xhr.responseType = "text";
    if (aData.dataType === "json") {
      xhr.responseType = "json";
    } else if (aData.dataType === "binary") {
      xhr.responseType = "blob";
    }

    xhr.addEventListener("load", function() {
      debug("Success loading " + href);
      if (xhr.status >= 200 && xhr.status < 400) {
        aMm.sendAsyncMessage("Webapps:GetLocalizationResource:Return",
          { requestID: aData.requestID, oid: aData.oid, data: xhr.response });
      } else {
        sendError("Error loading " + href, "UnavailableResource");
      }
    });
    xhr.addEventListener("error", function() {
      sendError("Error loading " + href, "UnavailableResource");
    });
    xhr.send(null);
  },

  
  checkManifest: function(aManifest) {
    if (!("languages-target" in aManifest)) {
      debug("Error: no 'languages-target' property.")
      return false;
    }

    if (!("languages-provided" in aManifest)) {
      debug("Error: no 'languages-provided' property.")
      return false;
    }

    for (let lang in aManifest["languages-provided"]) {
      let item = aManifest["languages-provided"][lang];

      if (!item.version) {
        debug("Error: missing 'version' in languages-provided." + lang);
        return false;
      }

      if (typeof item.version !== "number") {
        debug("Error: languages-provided." + lang +
              ".version must be a number but is a " + (typeof item.version));
        return false;
      }

      if (!item.apps) {
        debug("Error: missing 'apps' in languages-provided." + lang);
        return false;
      }

      for (let app in item.apps) {
        
        if (!isAbsoluteURI(app)) {
          debug("Error: languages-provided." + lang + "." + app +
                " must be an absolute manifest url.");
          return false;
        }

        if (typeof item.apps[app] !== "string") {
          debug("Error: languages-provided." + lang + ".apps." + app +
                " value must be a string but is " + (typeof item.apps[app]) +
                " : " + item.apps[app]);
          return false;
        }
      }
    }
    return true;
  },

  
  register: function(aApp, aManifest) {
    if (aApp.role !== "langpack") {
      
      return;
    }

    debug("register app " + aApp.manifestURL);

    if (!this.checkManifest(aManifest)) {
      debug("Invalid langpack manifest.");
      return;
    }

    let platformVersion = aManifest["languages-target"]
                                   ["app://*.gaiamobile.org/manifest.webapp"];
    let origin = Services.io.newURI(aApp.origin, null, null);

    for (let lang in aManifest["languages-provided"]) {
      let item = aManifest["languages-provided"][lang];
      let version = item.version;   
      let name = item.name || lang; 
      for (let app in item.apps) {
        let sendEvent = false;
        if (!this._data[app] ||
            !this._data[app].langs[lang] ||
            this._data[app].langs[lang].version > version) {
          if (!this._data[app]) {
            this._data[app] = {
              appId: this._appIdFromManifestURL(app),
              langs: {}
            };
          }
          this._data[app].langs[lang] = {
            version: version,
            target: platformVersion,
            name: name,
            url: origin.resolve(item.apps[app]),
            from: aApp.manifestURL
          }
          sendEvent = true;
          debug("Registered " + app + " -> " + uneval(this._data[app].langs[lang]));
        }

        
        
        if (sendEvent) {
          this.sendAppUpdate(app);
          ppmm.broadcastAsyncMessage(
            "Webapps:AdditionalLanguageChange",
            { manifestURL: app,
              languages: this.getAdditionalLanguages(app).langs });
        }
      }
    }
  },

  
  
  unregister: function(aApp, aManifest) {
    if (aApp.role !== "langpack") {
      
      return;
    }

    debug("unregister app " + aApp.manifestURL);

    for (let app in this._data) {
      let sendEvent = false;
      for (let lang in this._data[app].langs) {
        if (this._data[app].langs[lang].from == aApp.manifestURL) {
          sendEvent = true;
          delete this._data[app].langs[lang];
        }
      }
      
      
      if (sendEvent) {
        this.sendAppUpdate(app);
        ppmm.broadcastAsyncMessage(
            "Webapps:AdditionalLanguageChange",
            { manifestURL: app,
              languages: this.getAdditionalLanguages(app).langs });
      }
    }
  }
}

Langpacks.init();
