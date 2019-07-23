



































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function AboutSessionRestore() { }
AboutSessionRestore.prototype = {
  classDescription: "about:sessionrestore",
  contractID: "@mozilla.org/network/protocol/about;1?what=sessionrestore",
  classID: Components.ID("{7c65e6f0-7605-11dd-ad8b-0800200c9a66}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule]),
  
  getURIFlags: function(aURI) {
    return Ci.nsIAboutModule.ALLOW_SCRIPT;
  },
  
  newChannel: function(aURI) {
    let ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
    let channel = ios.newChannel("chrome://browser/content/aboutSessionRestore.xhtml",
                                 null, null);
    channel.originalURI = aURI;
    return channel;
  }
};

function NSGetModule(compMgr, fileSpec)
  XPCOMUtils.generateModule([AboutSessionRestore]);
