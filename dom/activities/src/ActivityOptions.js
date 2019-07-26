



"use strict";
 
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
 
function debug(aMsg) {
  
}





function ActivityOptions() {
  debug("ActivityOptions");
  this.wrappedJSObject = this;

  
  
  
  
  
  
  this._name = null;
  this._data = null;
}

ActivityOptions.prototype = {
  get name() {
    return this._name;
  },

  get data() {
    return this._data;
  },

  classID: Components.ID("{ee983dbb-d5ea-4c5b-be98-10a13cac9f9d}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDOMMozActivityOptions]),

  classInfo: XPCOMUtils.generateCI({
    classID: Components.ID("{ee983dbb-d5ea-4c5b-be98-10a13cac9f9d}"),
    contractID: "@mozilla.org/dom/activities/options;1",
    interfaces: [Ci.nsIDOMMozActivityOptions],
    flags: Ci.nsIClassInfo.DOM_OBJECT,
    classDescription: "Activity Options"
  })
}

const NSGetFactory = XPCOMUtils.generateNSGetFactory([ActivityOptions]);
