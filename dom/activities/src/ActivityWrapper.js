



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function debug(aMsg) {
  
}





function ActivityWrapper() {
  debug("ActivityWrapper");
}

ActivityWrapper.prototype = {
  wrapMessage: function wrapMessage(aMessage) {
    debug("Wrapping " + JSON.stringify(aMessage));
    let handler = Cc["@mozilla.org/dom/activities/request-handler;1"]
                    .createInstance(Ci.nsIDOMMozActivityRequestHandler);
    handler.wrappedJSObject._id = aMessage.id;

    
    var options = handler.wrappedJSObject._options;
    options.wrappedJSObject._name = aMessage.payload.name;
    options.wrappedJSObject._data = aMessage.payload.data;

    return handler;
  },

  classID: Components.ID("{5430d6f9-32d6-4924-ba39-6b6d1b093cd6}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsISystemMessagesWrapper])
}

const NSGetFactory = XPCOMUtils.generateNSGetFactory([ActivityWrapper]);

