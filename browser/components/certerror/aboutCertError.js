




































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function AboutCertError() {}
AboutCertError.prototype = {
  classDescription: "About Cert Error",
  contractID: "@mozilla.org/network/protocol/about;1?what=certerror",
  classID: Components.ID("{78d2286f-de9d-47ac-9c26-e8675aedf3be}"),
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

    var channel = ios.newChannel("chrome://browser/content/certerror/aboutCertError.xhtml",
                                 null, null);
    var principal = secMan.getCodebasePrincipal(aURI);

    channel.originalURI = aURI;
    channel.owner = principal;

    return channel;
  }
};

function NSGetModule(compMgr, fileSpec) {
  return XPCOMUtils.generateModule([AboutCertError]);
}
