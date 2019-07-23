



































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function AboutRights() {}
AboutRights.prototype = {
  classDescription: "about:rights",
  contractID: "@mozilla.org/network/protocol/about;1?what=rights",
  classID: Components.ID("{89e9da80-4c03-46a0-a357-cf77bbef98b9}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule]),
 
  getURIFlags: function(aURI) {
    return (Ci.nsIAboutModule.ALLOW_SCRIPT |
            Ci.nsIAboutModule.URI_SAFE_FOR_UNTRUSTED_CONTENT);
  },

  newChannel: function(aURI) {
    var ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);

    var secMan = Cc["@mozilla.org/scriptsecuritymanager;1"].
                 getService(Ci.nsIScriptSecurityManager);

    var channel = ios.newChannel("chrome://browser/content/aboutRights.xhtml",
                                 null, null);
    var principal = secMan.getCodebasePrincipal(aURI);

    channel.originalURI = aURI;
    channel.owner = principal;

    return channel;
  }
};

function NSGetModule(compMgr, fileSpec)
  XPCOMUtils.generateModule([AboutRights]);
