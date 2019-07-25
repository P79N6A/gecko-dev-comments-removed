






































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const MSG_INSTALL_ENABLED  = "WebInstallerIsInstallEnabled";
const MSG_INSTALL_ADDONS   = "WebInstallerInstallAddonsFromWebpage";
const MSG_INSTALL_CALLBACK = "WebInstallerInstallCallback";

Cu.import("resource://gre/modules/Services.jsm");










function InstallTriggerChild() {
  addEventListener("DOMWindowCreated", this, false);
}

var that = this;

InstallTriggerChild.prototype = {
  handleEvent: function handleEvent(aEvent) {
    var window = aEvent.originalTarget.defaultView.content;

    
    
    
    
    
    
    
    
    
    try {
      if (!window || !window.wrappedJSObject) {
        return;
      }
    }
    catch(e) {
      return;
    }

    
    
    if (window.wrappedJSObject.InstallTrigger)
        return;

    
    
    window.wrappedJSObject.InstallTrigger = {
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
      },

      

      SKIN: Ci.amIInstallTrigger.SKIN,
      LOCALE: Ci.amIInstallTrigger.LOCALE,
      CONTENT: Ci.amIInstallTrigger.CONTENT,
      PACKAGE: Ci.amIInstallTrigger.PACKAGE,

      


      enabled: function() {
        return sendSyncMessage(MSG_INSTALL_ENABLED, {
          mimetype: "application/x-xpinstall", referer: window.location.href
        })[0];
      },

      


      updateEnabled: function() {
        return this.enabled();
      },

      


      install: function(aArgs, aCallback) {
        var params = {
          mimetype: "application/x-xpinstall",
          referer: window.location.href,
          uris: [],
          hashes: [],
          names: [],
          icons: [],
        };

        for (var name in aArgs) {
          var item = aArgs[name];
          if (typeof item === 'string') {
            item = { URL: item };
          } else if (!("URL" in item)) {
            throw new Error("Missing URL property for '" + name + "'");
          }

          
          var url = this.resolveURL(item.URL);
          if (!this.checkLoadURIFromScript(url))
            throw new Error("insufficient permissions to install: " + url);

          var iconUrl = null;
          if ("IconURL" in item) {
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
        
        params.callbackId = this.addCallback(aCallback, params.uris);
        
        return sendSyncMessage(MSG_INSTALL_ADDONS, params)[0];
      },

      


      startSoftwareUpdate: function(aUrl, aFlags) {
        var url = Services.io.newURI(aUrl, null, null)
                          .QueryInterface(Ci.nsIURL).filename;
        var object = {};
        object[url] = { "URL": aUrl };
        return this.install(object);
      },

      


      installChrome: function(aType, aUrl, aSkin) {
        return this.startSoftwareUpdate(aUrl);
      },

      

      callbacks: {},

      













      addCallback: function(aCallback, aUrls) {
        if (!aCallback)
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

      








      resolveURL: function(aUrl) {
        return Services.io.newURI(aUrl, null,
                                  window.document.documentURIObject);
      },

      




      checkLoadURIFromScript: function(aUri) {
        var secman = Cc["@mozilla.org/scriptsecuritymanager;1"].
                     getService(Ci.nsIScriptSecurityManager);
        var principal = window.content.document.nodePrincipal;
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

    addMessageListener(MSG_INSTALL_CALLBACK,
      window.wrappedJSObject.InstallTrigger);
  },
};

new InstallTriggerChild();

