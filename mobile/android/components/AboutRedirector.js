


const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

let modules = {
  
  "": {
    uri: "chrome://browser/content/about.xhtml",
    privileged: true
  },

  
  
  fennec: {
    uri: "chrome://browser/content/about.xhtml",
    privileged: true,
    hide: true
  },
  get firefox() this.fennec,

  
  empty: {
    uri: "about:blank",
    privileged: false,
    hide: true
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
    privileged: false,
    hide: true
  },
  certerror: {
    uri: "chrome://browser/content/aboutCertError.xhtml",
    privileged: false,
    hide: true
  },
  home: {
    uri: "chrome://browser/content/aboutHome.xhtml",
    privileged: false
  },
  apps: {
    uri: "chrome://browser/content/aboutApps.xhtml",
    privileged: true
  },
  downloads: {
    uri: "chrome://browser/content/aboutDownloads.xhtml",
    privileged: true
  },
  reader: {
    uri: "chrome://browser/content/aboutReader.html",
    privileged: true,
    hide: true
  },
  readercontent: {
    uri: "chrome://browser/content/aboutReaderContent.html",
    privileged: false,
    hide: true
  },
  feedback: {
    uri: "chrome://browser/content/aboutFeedback.xhtml",
    privileged: true
  }
}

function AboutRedirector() {}
AboutRedirector.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule]),
  classID: Components.ID("{322ba47e-7047-4f71-aebf-cb7d69325cd9}"),

  _getModuleInfo: function (aURI) {
    let moduleName = aURI.path.replace(/[?#].*/, "").toLowerCase();
    return modules[moduleName];
  },

  
  getURIFlags: function(aURI) {
    let flags;
    let moduleInfo = this._getModuleInfo(aURI);
    if (moduleInfo.hide)
      flags = Ci.nsIAboutModule.HIDE_FROM_ABOUTABOUT;

    return flags | Ci.nsIAboutModule.ALLOW_SCRIPT;
  },

  newChannel: function(aURI) {
    let moduleInfo = this._getModuleInfo(aURI);

    var ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);

    var channel = ios.newChannel(moduleInfo.uri, null, null);
    
    if (!moduleInfo.privileged) {
      
      
      
      channel.owner = null;
    }

    channel.originalURI = aURI;

    return channel;
  }
};

const components = [AboutRedirector];
const NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
