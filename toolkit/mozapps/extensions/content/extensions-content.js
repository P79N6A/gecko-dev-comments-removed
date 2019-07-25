






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const MSG_INSTALL_ENABLED  = "WebInstallerIsInstallEnabled";
const MSG_INSTALL_ADDONS   = "WebInstallerInstallAddonsFromWebpage";
const MSG_INSTALL_CALLBACK = "WebInstallerInstallCallback";

var gIoService = Components.classes["@mozilla.org/network/io-service;1"]
                           .getService(Components.interfaces.nsIIOService);

function InstallTrigger(window) {
  this.window = window;
}

InstallTrigger.prototype = {
  __exposedProps__: {
    SKIN: "r",
    LOCALE: "r",
    CONTENT: "r",
    PACKAGE: "r",
    enabled: "r",
    updateEnabled: "r",
    install: "r",
    installChrome: "r",
    startSoftwareUpdate: "r",
    toSource: "r", 
  },

  

  SKIN: Ci.amIInstallTrigger.SKIN,
  LOCALE: Ci.amIInstallTrigger.LOCALE,
  CONTENT: Ci.amIInstallTrigger.CONTENT,
  PACKAGE: Ci.amIInstallTrigger.PACKAGE,

  


  enabled: function() {
    return sendSyncMessage(MSG_INSTALL_ENABLED, {
      mimetype: "application/x-xpinstall", referer: this.window.location.href
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
      referer: this.window.location.href,
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
    return gIoService.newURI(aUrl, null,
                             this.window.document.documentURIObject);
  },

  




  checkLoadURIFromScript: function(aUri) {
    var secman = Cc["@mozilla.org/scriptsecuritymanager;1"].
                 getService(Ci.nsIScriptSecurityManager);
    var principal = this.window.content.document.nodePrincipal;
    try {
      secman.checkLoadURIWithPrincipal(principal, aUri,
        Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL);
      return true;
    }
    catch(e) {
      return false;
    }
  },
};










function InstallTriggerManager() {
  this.callbacks = {};

  addMessageListener(MSG_INSTALL_CALLBACK, this);

  addEventListener("DOMWindowCreated", this, false);

  var self = this;
  addEventListener("unload", function() {
    
    self.callbacks = null;
  }, false);
}

InstallTriggerManager.prototype = {
  handleEvent: function handleEvent(aEvent) {
    var window = aEvent.target.defaultView;

    
    
    
    
    var uri = window.document.documentURIObject;
    if (uri.scheme === "chrome" || uri.spec.split(":")[0] == "about") {
      return;
    }

    window.wrappedJSObject.__defineGetter__("InstallTrigger", this.createInstallTrigger);
  },

  createInstallTrigger: function createInstallTrigger() {
    
    
    
    
    
    
    
    
    
    
    var obj = XPCNativeWrapper.unwrap(this);
    while (!obj.hasOwnProperty('InstallTrigger')) {
      obj = XPCNativeWrapper.unwrap(Object.getPrototypeOf(obj));
    }

    delete obj.InstallTrigger; 
    obj.InstallTrigger = new InstallTrigger(this);
    return obj.InstallTrigger;
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

