



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

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([RemoteTagServiceService]);
