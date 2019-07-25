





































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");






function BadCertHandler() {
}

BadCertHandler.prototype = {

  
  notifyCertProblem: function(socketInfo, status, targetSite) {
    return true;
  },

  
  notifySSLError: function(socketInfo, error, targetSite) {
    return true;
  },

  
  getInterface: function(iid) {
    return this.QueryInterface(iid);
  },

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIBadCertListener2,
                                         Ci.nsISSLErrorListener,
                                         Ci.nsIInterfaceRequestor]),

  classID:          Components.ID("{dbded6ec-edbf-4054-a834-287b82c260f9}"),
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([BadCertHandler]);
