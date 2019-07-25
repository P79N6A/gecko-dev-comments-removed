






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const MSG_INSTALL_ENABLED  = "WebInstallerIsInstallEnabled";
const MSG_INSTALL_ADDONS   = "WebInstallerInstallAddonsFromWebpage";
const MSG_INSTALL_CALLBACK = "WebInstallerInstallCallback";
const MSG_JAR_FLUSH        = "AddonJarFlush";

var gIoService = Components.classes["@mozilla.org/network/io-service;1"]
                           .getService(Components.interfaces.nsIIOService);

function createInstallTrigger(window) {
  let chromeObject = {
    window: window,
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

    


    enabled: function() {
      return sendSyncMessage(MSG_INSTALL_ENABLED, {
        mimetype: "application/x-xpinstall", referer: this.url.spec
      })[0];
    },

    


    updateEnabled: function() {
      return this.enabled();
    },

    


    install: function(aArgs, aCallback) {
      if (!aArgs || typeof aArgs != "object")
        throw new Error("Incorrect arguments passed to InstallTrigger.install()");

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
          throw new Error("Missing URL property for '" + name + "'");
        }

        
        var url = this.resolveURL(item.URL);
        if (!this.checkLoadURIFromScript(url))
          throw new Error("insufficient permissions to install: " + url);

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

    


    startSoftwareUpdate: function(aUrl, aFlags) {
      var url = gIoService.newURI(aUrl, null, null)
                          .QueryInterface(Ci.nsIURL).filename;
      var object = {};
      object[url] = { "URL": aUrl };
      return this.install(object);
    },

    


    installChrome: function(aType, aUrl, aSkin) {
      return this.startSoftwareUpdate(aUrl);
    },

    








    resolveURL: function(aUrl) {
      return gIoService.newURI(aUrl, null, this.url);
    },

    




    checkLoadURIFromScript: function(aUri) {
      var secman = Cc["@mozilla.org/scriptsecuritymanager;1"].
                   getService(Ci.nsIScriptSecurityManager);
      var principal = this.window.document.nodePrincipal;
      try {
        secman.checkLoadURIWithPrincipal(principal, aUri,
          Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL);
        return true;
      }
      catch(e) {
        return false;
      }
    }
  };

  let sandbox = Cu.Sandbox(window);
  let obj = Cu.evalInSandbox(
    "(function (x) {\
       var bind = Function.bind;\
       return {\
         enabled: bind.call(x.enabled, x),\
         updateEnabled: bind.call(x.updateEnabled, x),\
         install: bind.call(x.install, x),\
         installChrome: bind.call(x.installChrome, x),\
         startSoftwareUpdate: bind.call(x.startSoftwareUpdate, x)\
       };\
     })", sandbox)(chromeObject);

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
      
      addMessageListener(MSG_JAR_FLUSH, function(msg) {
        let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
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
  addEventListener("unload", function() {
    
    self.callbacks = null;
  }, false);
}

InstallTriggerManager.prototype = {
  handleEvent: function handleEvent(aEvent) {
    var window = aEvent.target.defaultView;

    window.wrappedJSObject.__defineGetter__("InstallTrigger", function() {
      
      
      
      

      delete window.wrappedJSObject.InstallTrigger;
      var installTrigger = createInstallTrigger(window);
      window.wrappedJSObject.InstallTrigger = installTrigger;
      return installTrigger;
    });
  },

  













  addCallback: function(aCallback, aUrls) {
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

  








  receiveMessage: function(aMessage) {
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

