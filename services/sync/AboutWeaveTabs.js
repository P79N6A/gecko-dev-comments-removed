



































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");

function AboutWeaveTabs() {}
AboutWeaveTabs.prototype = {
  classDescription: "about:sync-tabs",
  contractID: "@mozilla.org/network/protocol/about;1?what=sync-tabs",
  classID: Components.ID("{ecb6987d-9d71-475d-a44d-a5ff2099b08c}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule,
                                         Ci.nsISupportsWeakReference]),

  getURIFlags: function(aURI) {
    return (Ci.nsIAboutModule.ALLOW_SCRIPT);
  },

  newChannel: function(aURI) {
    let ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);
    let ch = ios.newChannel("chrome://weave/content/firefox/tabs.xul", null, null);
    ch.originalURI = aURI;
    return ch;
  }
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([ AboutWeaveTabs ]);
}

