





"use strict";

(function(){

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const MSG_INSTALL_ENABLED  = "WebInstallerIsInstallEnabled";
const MSG_INSTALL_ADDONS   = "WebInstallerInstallAddonsFromWebpage";
const MSG_INSTALL_CALLBACK = "WebInstallerInstallCallback";
const MSG_JAR_FLUSH        = "AddonJarFlush";

var gIoService = Components.classes["@mozilla.org/network/io-service;1"]
                           .getService(Components.interfaces.nsIIOService);

function createInstallTrigger(window) {
  let chromeObject = {
    principal: window.document.nodePrincipal,
    url: window.document.documentURIObject,

    __exposedProps__: {
      SKIN: "r",
      LOCALE: "r",
      CONTENT: "r",
      PACKAGE: "r",
      enabled: "r",
      updateEnabled: "r",
      install: "r",
      installChrome: "r",
      startSoftwareUpdate: "r"
    },

    

    SKIN: Ci.amIInstallTrigger.SKIN,
    LOCALE: Ci.amIInstallTrigger.LOCALE,
    CONTENT: Ci.amIInstallTrigger.CONTENT,
    PACKAGE: Ci.amIInstallTrigger.PACKAGE,

    


    enabled: function createInstallTrigger_enabled() {
      return sendSyncMessage(MSG_INSTALL_ENABLED, {
        mimetype: "application/x-xpinstall", referer: this.url.spec
      })[0];
    },

    


    updateEnabled: function createInstallTrigger_updateEnabled() {
      return this.enabled();
    },

    


    install: function createInstallTrigger_install(aArgs, aCallback) {
      if (!aArgs || typeof aArgs != "object")
        throw Components.Exception("Incorrect arguments passed to InstallTrigger.install()",
                                   Cr.NS_ERROR_INVALID_ARGS);

      var params = {
        installerId: this.installerId,
        mimetype: "application/x-xpinstall",
        referer: this.url.spec,
        uris: [],
        hashes: [],
        names: [],
        icons: [],
      };

      for (var name in aArgs) {
        var item = aArgs[name];
        if (typeof item === 'string') {
          item = { URL: item };
        } else if (!("URL" in item) || item.URL === undefined) {
          throw Components.Exception("Missing URL property for '" + name + "'");
        }

        
        var url = this.resolveURL(item.URL);
        if (!this.checkLoadURIFromScript(url))
          throw Components.Exception("Insufficient permissions to install: " + url);

        var iconUrl = null;
        if ("IconURL" in item && item.IconURL !== undefined) {
          iconUrl = this.resolveURL(item.IconURL);
          if (!this.checkLoadURIFromScript(iconUrl)) {
            iconUrl = null; 
          }
        }
        params.uris.push(url.spec);
        params.hashes.push("Hash" in item ? item.Hash : null);
        params.names.push(name);
        params.icons.push(iconUrl ? iconUrl.spec : null);
      }
      
      params.callbackId = manager.addCallback(aCallback, params.uris);
      
      return sendSyncMessage(MSG_INSTALL_ADDONS, params)[0];
    },

    


    startSoftwareUpdate: function createInstallTrigger_startSoftwareUpdate(aUrl, aFlags) {
      var url = gIoService.newURI(aUrl, null, null)
                          .QueryInterface(Ci.nsIURL).filename;
      var object = {};
      object[url] = { "URL": aUrl };
      return this.install(object);
    },

    


    installChrome: function createInstallTrigger_installChrome(aType, aUrl, aSkin) {
      return this.startSoftwareUpdate(aUrl);
    },

    








    resolveURL: function createInstallTrigger_resolveURL(aUrl) {
      return gIoService.newURI(aUrl, null, this.url);
    },

    




    checkLoadURIFromScript: function createInstallTrigger_checkLoadURIFromScript(aUri) {
      var secman = Cc["@mozilla.org/scriptsecuritymanager;1"].
                   getService(Ci.nsIScriptSecurityManager);
      try {
        secman.checkLoadURIWithPrincipal(this.principal, aUri,
          Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL);
        return true;
      }
      catch(e) {
        return false;
      }
    }
  };

  let obj = Cu.createObjectIn(window);
  function genPropDesc(fun) {
    return { enumerable: true, configurable: true, writable: true,
             value: chromeObject[fun].bind(chromeObject) };
  }
  const properties = {
    'enabled': genPropDesc('enabled'),
    'updateEnabled': genPropDesc('updateEnabled'),
    'install': genPropDesc('install'),
    'installChrome': genPropDesc('installChrome'),
    'startSoftwareUpdate': genPropDesc('startSoftwareUpdate')
  };

  Object.defineProperties(obj, properties);

  Cu.makeObjectPropsNormal(obj);

  obj.SKIN = chromeObject.SKIN;
  obj.LOCALE = chromeObject.LOCALE;
  obj.CONTENT = chromeObject.CONTENT;
  obj.PACKAGE = chromeObject.PACKAGE;

  return obj;
};










function InstallTriggerManager() {
  this.callbacks = {};

  addMessageListener(MSG_INSTALL_CALLBACK, this);

  try {
    
    if (Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).processType !== Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT) {
      
      addMessageListener(MSG_JAR_FLUSH, function jar_flushMessageListener(msg) {
        let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
        file.initWithPath(msg.json);
        Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService)
          .notifyObservers(file, "flush-cache-entry", null);
      });
    }
  } catch(e) {
    Cu.reportError(e);
  }
    
  addEventListener("DOMWindowCreated", this, false);

  var self = this;
  addEventListener("unload", function unloadEventListener() {
    
    self.callbacks = null;
  }, false);
}

InstallTriggerManager.prototype = {
  handleEvent: function ITM_handleEvent(aEvent) {
    var window = aEvent.target.defaultView;

    window.wrappedJSObject.__defineGetter__("InstallTrigger", function installTriggerGetter() {
      
      
      
      

      delete window.wrappedJSObject.InstallTrigger;
      var installTrigger = createInstallTrigger(window);
      window.wrappedJSObject.InstallTrigger = installTrigger;
      return installTrigger;
    });
  },

  













  addCallback: function ITM_addCallback(aCallback, aUrls) {
    if (!aCallback || typeof aCallback != "function")
      return -1;
    var callbackId = 0;
    while (callbackId in this.callbacks)
      callbackId++;
    this.callbacks[callbackId] = {
      callback: aCallback,
      urls: aUrls.slice(0), 
                            
                            
    };
    return callbackId;
  },

  








  receiveMessage: function ITM_receiveMessage(aMessage) {
    var payload = aMessage.json;
    var callbackId = payload.callbackId;
    var url = payload.url;
    var status = payload.status;
    var callbackObj = this.callbacks[callbackId];
    if (!callbackObj)
      return;
    try {
      callbackObj.callback(url, status);
    }
    catch (e) {
      dump("InstallTrigger callback threw an exception: " + e + "\n");
    }
    callbackObj.urls.splice(callbackObj.urls.indexOf(url), 1);
    if (callbackObj.urls.length == 0)
      this.callbacks[callbackId] = null;
  },
};

var manager = new InstallTriggerManager();

})();
