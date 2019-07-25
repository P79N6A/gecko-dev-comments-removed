



































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function WebappsRegistry() {
  this.messages = ["Webapps:Install:Return:OK", "Webapps:Install:Return:KO",
                   "Webapps:Uninstall:Return:OK", "Webapps:Uninstall:Return:KO",
                   "Webapps:Enumerate:Return:OK", "Webapps:Enumerate:Return:KO"];

  this.mm = Cc["@mozilla.org/childprocessmessagemanager;1"].getService(Ci.nsIFrameMessageManager);

  this.messages.forEach((function(msgName) {
    this.mm.addMessageListener(msgName, this);
  }).bind(this));

  this._window = null;
  this._id = this._getRandomId();
  this._callbacks = [];
}

WebappsRegistry.prototype = {
  _onerror: null,
  _oninstall: null,
  _onuninstall: null,

  


  checkManifest: function(aManifest, aInstallOrigin) {
    
    if (aManifest.name == undefined)
      return false;
    
    if (aManifest.installs_allowed_from) {
      ok = false;
      aManifest.installs_allowed_from.forEach(function(aOrigin) {
        if (aOrigin == "*" || aOrigin == aInstallOrigin)
          ok = true;
      });
      return ok;
    }
    return true;
  },
  
  getCallbackId: function(aCallback) {
    let id = "id" + this._getRandomId();
    this._callbacks[id] = aCallback;
    return id;
  },
  
  getCallback: function(aId) {
    return this._callbacks[aId];
  },

  removeCallback: function(aId) {
    if (this._callbacks[aId])
      delete this._callbacks[aId];
  },
  
  _getRandomId: function() {
    return Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator).generateUUID().toString();
  },

  _convertAppsArray: function(aApps) {
    let apps = new Array();
    for (let i = 0; i < aApps.length; i++) {
      let app = aApps[i];
      apps.push(new WebappsApplication(app.origin, app.manifest, app.receipt, app.installOrigin, app.installTime));
    }
    return apps;
  },

  set oninstall(aCallback) {
    if (this.hasPrivileges)
      this._oninstall = aCallback;
    else
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },
  
  set onuninstall(aCallback) {
    if (this.hasPrivileges)
      this._onuninstall = aCallback;
    else
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  set onerror(aCallback) {
    this._onerror = aCallback;
  },

  receiveMessage: function(aMessage) {
    let msg = aMessage.json;
    if (!(msg.oid == this._id || aMessage.name == "Webapps:Install:Return:OK" || aMessage.name == "Webapps:Uninstall:Return:OK"))
      return
    let app = msg.app;
    let cb;
    switch (aMessage.name) {
      case "Webapps:Install:Return:OK":
        if (this._oninstall)
          this._oninstall.handleEvent(new WebappsApplication(app.origin, app.manifest, app.receipt,
                                                app.installOrigin, app.installTime));
        break;
      case "Webapps:Install:Return:KO":
        if (this._onerror)
          this._onerror.handleEvent(new RegistryError(Ci.nsIDOMApplicationRegistryError.DENIED));
        break;
      case "Webapps:Uninstall:Return:OK":
        if (this._onuninstall)
          this._onuninstall.handleEvent(new WebappsApplication(msg.origin, null, null, null, 0));
        break;
      case "Webapps:Uninstall:Return:KO":
        if (this._onerror)
          this._onerror.handleEvent(new RegistryError(Ci.nsIDOMApplicationRegistryError.PERMISSION_DENIED));
        break;
      case "Webapps:Enumerate:Return:OK":
        cb = this.getCallback(msg.callbackID);
        if (cb.success) {
          let apps = this._convertAppsArray(msg.apps);
          cb.success.handleEvent(apps, apps.length);
        }
        break;
      case "Webapps:Enumerate:Return:KO":
        cb = this.getCallback(msg.callbackID);
        if (cb.error)
          cb.error.handleEvent(new RegistryError(Ci.nsIDOMApplicationRegistryError.PERMISSION_DENIED));
        break;
    }
    this.removeCallback(msg.callbackID);
  },
  
  _fireError: function(aCode) {
    if (!this._onerror)
      return;
    this._onerror.handleEvent(new RegistryError(aCode));
  },

  _getOrigin: function(aURL) {
    let uri = Services.io.newURI(aURL, null, null);
    return uri.prePath; 
  },

  
  
  install: function(aURL, aReceipt) {
    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].createInstance(Ci.nsIXMLHttpRequest);
    xhr.open("GET", aURL, true);

    xhr.addEventListener("load", (function() {
      if (xhr.status == 200) {
        try {
          let installOrigin = this._getOrigin(this._window.location.href);
          let manifest = JSON.parse(xhr.responseText, installOrigin);
          if (!this.checkManifest(manifest, installOrigin)) {
            this._fireError(Ci.nsIDOMApplicationRegistryError.INVALID_MANIFEST);
          } else {
            this.mm.sendAsyncMessage("Webapps:Install", { app: { installOrigin: installOrigin,
                                                          origin: this._getOrigin(aURL),
                                                          manifest: manifest,
                                                          receipt: aReceipt },
                                                          from: this._window.location.href,
                                                          oid: this._id });
          }
        } catch(e) {
          this._fireError(Ci.nsIDOMApplicationRegistryError.MANIFEST_PARSE_ERROR);
        }
      }
      else {
        this._fireError(Ci.nsIDOMApplicationRegistryError.MANIFEST_URL_ERROR);
      }      
    }).bind(this), false);

    xhr.addEventListener("error", (function() {
      this._fireError(Ci.nsIDOMApplicationRegistryError.NETWORK_ERROR);
    }).bind(this), false);

    xhr.send(null);
  },

  uninstall: function(aOrigin) {
    if (this.hasPrivileges)
      this.mm.sendAsyncMessage("Webapps:Uninstall", { from: this._window.location.href,
                                                      origin: aOrigin,
                                                      oid: this._id });
    else
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  launch: function(aOrigin) {
    this.mm.sendAsyncMessage("Webapps:Launch", { origin: aOrigin,
                                                 from: this._window.location.href});
  },
  
  enumerate: function(aSuccess, aError) {
    this.mm.sendAsyncMessage("Webapps:Enumerate", { from: this._window.location.href,
                                                    origin: this._getOrigin(this._window.location.href),
                                                    oid: this._id,
                                                    callbackID:  this.getCallbackId({ success: aSuccess, error: aError }) });
  },

  handleEvent: function(aEvent) {
    if (aEvent.type == "unload") {
      
      try {
        this._oninstall = null;
        this._onuninstall = null;
        this._onerror = null;
        this._callbacks = [];
      } catch(e) {
        dump("WebappsRegistry error:" + e + "\n");
      }
    }
  },
  
  
  init: function(aWindow) {
    dump("DOMApplicationRegistry::init() " + aWindow + "\n");
    this._window = aWindow;
    this._window.addEventListener("unload", this, false);
    this._window.appId = this._id;
    let from = Services.io.newURI(this._window.location.href, null, null);
    let perm = Services.perms.testExactPermission(from, "webapps-manage");

    
    this.hasPrivileges = perm == Ci.nsIPermissionManager.ALLOW_ACTION || from.schemeIs("chrome") || from.schemeIs("about");
  },
  
  classID: Components.ID("{fff440b3-fae2-45c1-bf03-3b5a2e432270}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMApplicationRegistry, Ci.nsIDOMGlobalPropertyInitializer]),
  
  classInfo: XPCOMUtils.generateCI({classID: Components.ID("{fff440b3-fae2-45c1-bf03-3b5a2e432270}"),
                                    contractID: "@mozilla.org/webapps;1",
                                    interfaces: [Ci.nsIDOMApplicationRegistry],
                                    flags: Ci.nsIClassInfo.DOM_OBJECT,
                                    classDescription: "Webapps Registry"})
}

function WebappsApplication(aOrigin, aManifest, aReceipt, aInstallOrigin, aInstallTime) {
  this._origin = aOrigin;
  this._manifest = aManifest;
  this._receipt = aReceipt;
  this._installOrigin = aInstallOrigin;
  this._installTime = aInstallTime;
}

WebappsApplication.prototype = {
  _origin: null,
  _manifest: null,
  _receipt: null,
  _installOrigin: null,
  _installTime: 0,

  get origin() {
    return this._origin;
  },

  get manifest() {
    return this._manifest;
  },

  get receipt() {
    return this._receipt;
  },

  get installOrigin() {
    return this._installOrigin;
  },
  
  get installTime() {
    return this._installTime;
  },

  classID: Components.ID("{723ed303-7757-4fb0-b261-4f78b1f6bd22}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMApplication]),

  classInfo: XPCOMUtils.generateCI({classID: Components.ID("{723ed303-7757-4fb0-b261-4f78b1f6bd22}"),
                                    contractID: "@mozilla.org/webapps/application;1",
                                    interfaces: [Ci.nsIDOMApplication],
                                    flags: Ci.nsIClassInfo.DOM_OBJECT,
                                    classDescription: "Webapps Application"})
}

function RegistryError(aCode) {
  this._code = aCode;
}

RegistryError.prototype = {
  _code: null,
  
  get code() {
    return this._code;
  },
  
  classID: Components.ID("{b4937718-11a3-400b-a69f-ab442a418569}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMApplicationRegistryError]),

  classInfo: XPCOMUtils.generateCI({classID: Components.ID("{b4937718-11a3-400b-a69f-ab442a418569}"),
                                    contractID: "@mozilla.org/webapps/error;1",
                                    interfaces: [Ci.nsIDOMApplicationRegistryError],
                                    flags: Ci.nsIClassInfo.DOM_OBJECT,
                                    classDescription: "Webapps Registry Error"})
}

const NSGetFactory = XPCOMUtils.generateNSGetFactory([WebappsRegistry, WebappsApplication, RegistryError]);

