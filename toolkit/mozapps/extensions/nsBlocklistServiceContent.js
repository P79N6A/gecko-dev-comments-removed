



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");

const kMissingAPIMessage = "Unsupported blocklist call in the child process."








function Blocklist() {
}

Blocklist.prototype = {
  classID: Components.ID("{e0a106ed-6ad4-47a4-b6af-2f1c8aa4712d}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIBlocklistService]),

  





  flattenObject: function (aTag) {
    
    
    let props = ["name", "description", "filename", "version"];
    let dataWrapper = {};
    for (let prop of props) {
      dataWrapper[prop] = aTag[prop];
    }
    return dataWrapper;
  },

  
  

  isAddonBlocklisted: function (aAddon, aAppVersion, aToolkitVersion) {
    throw new Error(kMissingAPIMessage);
  },

  getAddonBlocklistState: function (aAddon, aAppVersion, aToolkitVersion) {
    throw new Error(kMissingAPIMessage);
  },

  getPluginBlocklistState: function (aPluginTag, aAppVersion, aToolkitVersion) {
    return Services.cpmm.sendSyncMessage("Blocklist::getPluginBlocklistState", {
      addonData: this.flattenObject(aPluginTag),
      appVersion: aAppVersion,
      toolkitVersion: aToolkitVersion
    })[0];
  },

  getAddonBlocklistURL: function (aAddon, aAppVersion, aToolkitVersion) {
    throw new Error(kMissingAPIMessage);
  },

  getPluginBlocklistURL: function (aPluginTag) {
    throw new Error(kMissingAPIMessage);
  },

  getPluginInfoURL: function (aPluginTag) {
    throw new Error(kMissingAPIMessage);
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([Blocklist]);
