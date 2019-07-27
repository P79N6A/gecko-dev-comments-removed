 



const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

let modules = {
  newtab: {
    uri: "chrome://browser/content/Start.xul",
    privileged: true
  },
  
  empty: {
    uri: "about:blank",
    privileged: false
  },
  firstrun: {
    uri: "chrome://browser/content/firstrun/firstrun.xhtml",
    privileged: true
  },
  rights: {
#ifdef MOZ_OFFICIAL_BRANDING
    uri: "chrome://browser/content/aboutRights.xhtml",
#else
    uri: "chrome://global/content/aboutRights-unbranded.xhtml",
#endif
    privileged: false
  },
  blocked: {
    uri: "chrome://browser/content/blockedSite.xhtml",
    privileged: true
  },
  certerror: {
    uri: "chrome://browser/content/aboutCertError.xhtml",
    privileged: true
  },
  start: {
    uri: "about:newtab",
    privileged: true
  },
  home: {
    uri: "about:newtab",
    privileged: true
  },
#ifdef MOZ_CRASHREPORTER
  crashprompt: {
    uri: "chrome://browser/content/crashprompt.xhtml",
    privileged: true
  },
#endif
}

function AboutGeneric() {}

AboutGeneric.prototype = {
  classID: Components.ID("{433d2d75-5923-49b0-854d-f37267b03dc7}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule]),

  _getModuleInfo: function (aURI) {
    let moduleName = aURI.path.replace(/[?#].*/, "").toLowerCase();
    return modules[moduleName];
  },

  getURIFlags: function(aURI) {
    return Ci.nsIAboutModule.ALLOW_SCRIPT;
  },

  newChannel: function(aURI, aLoadInfo) {
    let moduleInfo = this._getModuleInfo(aURI);

    var ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);

    var newURI = ios.newURI(moduleInfo.uri, null, null);
    var channel = ios.newChannelFromURIWithLoadInfo(newURI, aLoadInfo);

    if (!moduleInfo.privileged) {
      
      
      
      channel.owner = null;
    }

    channel.originalURI = aURI;
    return channel;
  }
};

const components = [AboutGeneric];
const NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
