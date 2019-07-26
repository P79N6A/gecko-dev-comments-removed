



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsISyncMessageSender");

function debug(aMsg) {
  
}





function ActivityRequestHandler() {
  debug("ActivityRequestHandler");
  this.wrappedJSObject = this;

  
  
  
  
  
  
  this._id = null;
  this._options = Cc["@mozilla.org/dom/activities/options;1"]
                    .createInstance(Ci.nsIDOMMozActivityOptions);
}

ActivityRequestHandler.prototype = {
  __exposedProps__: {
                      source: "r",
                      postResult: "r",
                      postError: "r"
                    },

  get source() {
    return this._options;
  },

  postResult: function arh_postResult(aResult) {
    cpmm.sendAsyncMessage("Activity:PostResult", {
      "id": this._id,
      "result": aResult
    });
  },

  postError: function arh_postError(aError) {
    cpmm.sendAsyncMessage("Activity:PostError", {
      "id": this._id,
      "error": aError
    });
  },

  classID: Components.ID("{9326952a-dbe3-4d81-a51f-d9c160d96d6b}"),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIDOMMozActivityRequestHandler
  ]),

  classInfo: XPCOMUtils.generateCI({
    classID: Components.ID("{9326952a-dbe3-4d81-a51f-d9c160d96d6b}"),
    contractID: "@mozilla.org/dom/activities/request-handler;1",
    interfaces: [Ci.nsIDOMMozActivityRequestHandler],
    flags: Ci.nsIClassInfo.DOM_OBJECT,
    classDescription: "Activity Request Handler"
  })
}

const NSGetFactory = XPCOMUtils.generateNSGetFactory([ActivityRequestHandler]);
