



"use strict"

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Messaging.jsm");

function ActivitiesGlue() { }

ActivitiesGlue.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIActivityUIGlue]),
  classID: Components.ID("{e4deb5f6-d5e3-4fce-bc53-901dd9951c48}"),

  
  chooseActivity: function ap_chooseActivity(aOptions, aActivities, aCallback) {
    Messaging.sendRequestForResult({
      type: "WebActivity:Open",
      activity: { name: aOptions.name, data: aOptions.data }
    }).then((result) => {
      aCallback.handleEvent(Ci.nsIActivityUIGlueCallback.NATIVE_ACTIVITY, result);
    });
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ActivitiesGlue]);
