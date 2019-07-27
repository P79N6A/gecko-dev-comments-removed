




"use strict";

this.EXPORTED_SYMBOLS = [
  "ContentPref",
  "cbHandleResult",
  "cbHandleError",
  "cbHandleCompletion",
  "safeCallback",
];

const { interfaces: Ci, classes: Cc, results: Cr, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function ContentPref(domain, name, value) {
  this.domain = domain;
  this.name = name;
  this.value = value;
}

ContentPref.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentPref]),
};

function cbHandleResult(callback, pref) {
  safeCallback(callback, "handleResult", [pref]);
}

function cbHandleCompletion(callback, reason) {
  safeCallback(callback, "handleCompletion", [reason]);
}

function cbHandleError(callback, nsresult) {
  safeCallback(callback, "handleError", [nsresult]);
}

function safeCallback(callbackObj, methodName, args) {
  if (!callbackObj || typeof(callbackObj[methodName]) != "function")
    return;
  try {
    callbackObj[methodName].apply(callbackObj, args);
  }
  catch (err) {
    Cu.reportError(err);
  }
}
