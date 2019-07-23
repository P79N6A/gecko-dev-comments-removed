



































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function AboutPrivateBrowsing() { }
AboutPrivateBrowsing.prototype = {
  classDescription: "about:privatebrowsing",
  contractID: "@mozilla.org/network/protocol/about;1?what=privatebrowsing",
  classID: Components.ID("{d92a18c8-234d-49e4-9936-3b7e020c29a2}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule]),
  
  getURIFlags: function(aURI) {
    return Ci.nsIAboutModule.ALLOW_SCRIPT;
  },
  
  newChannel: function(aURI) {
    let ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    let channel = ios.newChannel("chrome://browser/content/aboutPrivateBrowsing.xhtml",
                                 null, null);
    channel.originalURI = aURI;
    return channel;
  }
};

function NSGetModule(compMgr, fileSpec)
  XPCOMUtils.generateModule([AboutPrivateBrowsing]);
