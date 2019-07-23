



































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

function AboutRobots() {}
AboutRobots.prototype = {
  classDescription: "About Robots",
  contractID: "@mozilla.org/network/protocol/about;1?what=robots",
  classID: Components.ID("{e18da21c-a4b8-4be5-98aa-942e1e19f35c}"),
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

    var channel = ios.newChannel("chrome://browser/content/aboutRobots.xhtml",
                                 null, null);
    var principal = secMan.getCodebasePrincipal(aURI);

    channel.originalURI = aURI;
    channel.owner = principal;

    return channel;
  }
};

function NSGetModule(compMgr, fileSpec)
  XPCOMUtils.generateModule([AboutRobots]);
