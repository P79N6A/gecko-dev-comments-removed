











"use strict";

const Cu = Components.utils;
const Ci = Components.interfaces;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function RemoteTagServiceService()
{
}

RemoteTagServiceService.prototype = {
  classID: Components.ID("{dfd07380-6083-11e4-9803-0800200c9a66}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRemoteTagService, Ci.nsISupportsWeakReference]),

  





  getRemoteObjectTag: function(target) {
    if (target instanceof Ci.nsIDocShellTreeItem) {
      return "ContentDocShellTreeItem";
    }

    if (target instanceof Ci.nsIDOMDocument) {
      return "ContentDocument";
    }

    return "generic";
  }
};

function AddonPolicyService()
{
  this.wrappedJSObject = this;
  this.mayLoadURICallbacks = new Map();
}

AddonPolicyService.prototype = {
  classID: Components.ID("{89560ed3-72e3-498d-a0e8-ffe50334d7c5}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAddonPolicyService]),

  






  addonMayLoadURI(aAddonId, aURI) {
    let cb = this.mayLoadURICallbacks[aAddonId];
    return cb ? cb(aURI) : false;
  },

  





  extensionURILoadableByAnyone(aURI) {
    if (aURI.scheme != "moz-extension") {
      throw new TypeError("non-extension URI passed");
    }

    let cb = this.extensionURILoadCallback;
    return cb ? cb(aURI) : false;
  },

  




  setAddonLoadURICallback(aAddonId, aCallback) {
    this.mayLoadURICallbacks[aAddonId] = aCallback;
  },

  




  setExtensionURILoadCallback(aCallback) {
    var old = this.extensionURILoadCallback;
    this.extensionURILoadCallback = aCallback;
    return old;
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([RemoteTagServiceService, AddonPolicyService]);
