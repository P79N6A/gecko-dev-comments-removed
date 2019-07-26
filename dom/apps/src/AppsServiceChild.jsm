



"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;




this.EXPORTED_SYMBOLS = ["DOMApplicationRegistry"];

Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function debug(s) {
  
}

this.DOMApplicationRegistry = {
  init: function init() {
    debug("init");
    this.cpmm = Cc["@mozilla.org/childprocessmessagemanager;1"]
                  .getService(Ci.nsISyncMessageSender);

    ["Webapps:AddApp", "Webapps:RemoveApp"].forEach((function(aMsgName) {
      this.cpmm.addMessageListener(aMsgName, this);
    }).bind(this));

    
    
    this.webapps = this.cpmm.sendSyncMessage("Webapps:GetList", { })[0];
    Services.obs.addObserver(this, "xpcom-shutdown", false);
  },

  observe: function(aSubject, aTopic, aData) {
    
    
    this.webapps = null;
    ["Webapps:AddApp", "Webapps:RemoveApp"].forEach((function(aMsgName) {
      this.cpmm.removeMessageListener(aMsgName, this);
    }).bind(this));
  },

  receiveMessage: function receiveMessage(aMessage) {
    debug("Received " + aMessage.name + " message.");
    let msg = aMessage.json;
    switch (aMessage.name) {
      case "Webapps:AddApp":
        this.webapps[msg.id] = msg.app;
        break;
      case "Webapps:RemoveApp":
        delete this.webapps[msg.id];
        break;
    }
  },

  getAppByManifestURL: function getAppByManifestURL(aManifestURL) {
    debug("getAppByManifestURL " + aManifestURL);
    return AppsUtils.getAppByManifestURL(this.webapps, aManifestURL);
  },

  getAppLocalIdByManifestURL: function getAppLocalIdByManifestURL(aManifestURL) {
    debug("getAppLocalIdByManifestURL " + aManifestURL);
    return AppsUtils.getAppLocalIdByManifestURL(this.webapps, aManifestURL);
  },

  getCSPByLocalId: function(aLocalId) {
    debug("getCSPByLocalId:" + aLocalId);
    return AppsUtils.getCSPByLocalId(this.webapps, aLocalId);
  },

  getAppByLocalId: function getAppByLocalId(aLocalId) {
    debug("getAppByLocalId " + aLocalId);
    return AppsUtils.getAppByLocalId(this.webapps, aLocalId);
  },

  getManifestURLByLocalId: function getManifestURLByLocalId(aLocalId) {
    debug("getManifestURLByLocalId " + aLocalId);
    return AppsUtils.getManifestURLByLocalId(this.webapps, aLocalId);
  },

  getAppFromObserverMessage: function getAppFromObserverMessage(aMessage) {
    debug("getAppFromObserverMessage " + aMessage);
    return AppsUtils.getAppFromObserverMessage(this.webapps. aMessage);
  }
}

DOMApplicationRegistry.init();
