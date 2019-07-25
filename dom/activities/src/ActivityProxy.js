



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/ObjectWrapper.jsm");

XPCOMUtils.defineLazyGetter(this, "cpmm", function() {
  return Cc["@mozilla.org/childprocessmessagemanager;1"]
           .getService(Ci.nsIFrameMessageManager)
           .QueryInterface(Ci.nsISyncMessageSender);
});

function debug(aMsg) {
  
}







function ActivityProxy() {
  debug("ActivityProxy");
  this.activity = null;
}

ActivityProxy.prototype = {
  startActivity: function actProxy_startActivity(aActivity, aOptions, aWindow) {
    debug("startActivity");

    this.window = aWindow;
    this.activity = aActivity;
    this.id = Cc["@mozilla.org/uuid-generator;1"]
                .getService(Ci.nsIUUIDGenerator)
                .generateUUID().toString();
    cpmm.sendAsyncMessage("Activity:Start", { id: this.id, options: aOptions });

    cpmm.addMessageListener("Activity:FireSuccess", this);
    cpmm.addMessageListener("Activity:FireError", this);
  },

  receiveMessage: function actProxy_receiveMessage(aMessage) {
    debug("Got message: " + aMessage.name);
    let msg = aMessage.json;
    if (msg.id != this.id)
      return;
    debug("msg=" + JSON.stringify(msg));

    switch(aMessage.name) {
      case "Activity:FireSuccess":
        debug("FireSuccess");
        Services.DOMRequest.fireSuccess(this.activity,
                                        ObjectWrapper.wrap(msg.result, this.window));
        break;
      case "Activity:FireError":
        debug("FireError");
        Services.DOMRequest.fireError(this.activity, msg.error);
        break;
    }
  },

  cleanup: function actProxy_cleanup() {
    debug("cleanup");
    cpmm.removeMessageListener("Activity:FireSuccess", this);
    cpmm.removeMessageListener("Activity:FireError", this);
  },

  classID: Components.ID("{ba9bd5cb-76a0-4ecf-a7b3-d2f7c43c5949}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIActivityProxy])
}

const NSGetFactory = XPCOMUtils.generateNSGetFactory([ActivityProxy]);
