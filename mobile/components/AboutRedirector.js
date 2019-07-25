




































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

let modules = {
  fennec: {
    uri: "chrome://browser/content/about.xhtml",
    privileged: true
  },
  
  get firefox() this.fennec,

  firstrun: {
    uri: "chrome://browser/content/firstrun/firstrun.xhtml",
    privileged: true
  },
  rights: {
#ifdef MOZ_OFFICIAL_BRANDING
    uri: "chrome://global/content/aboutRights.xhtml",
#else
    uri: "chrome://global/content/aboutRights-unbranded.xhtml",
#endif
    privileged: false
  },
  certerror: {
    uri: "chrome://browser/content/aboutCertError.xhtml",
    privileged: true
  },
  home: {
    uri: "chrome://browser/content/aboutHome.xhtml",
    privileged: true
  },
  "sync-tabs": {
    uri: "chrome://browser/content/aboutTabs.xhtml",
    privileged: true
  }
}

function AboutGeneric() {}
AboutGeneric.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAboutModule]),

  _getModuleInfo: function (aURI) {
    let moduleName = aURI.path.replace(/[?#].*/, "").toLowerCase();
    return modules[moduleName];
  },

  
  getURIFlags: function(aURI) {
    return Ci.nsIAboutModule.ALLOW_SCRIPT;
  },

  newChannel: function(aURI) {
    let moduleInfo = this._getModuleInfo(aURI);

    var ios = Cc["@mozilla.org/network/io-service;1"].
              getService(Ci.nsIIOService);

    var channel = ios.newChannel(moduleInfo.uri, null, null);
    
    if (!moduleInfo.privileged) {
      let secMan = Cc["@mozilla.org/scriptsecuritymanager;1"].
                   getService(Ci.nsIScriptSecurityManager);
      let principal = secMan.getCodebasePrincipal(aURI);
      channel.owner = principal;
    }

    channel.originalURI = aURI;

    return channel;
  }
};

function AboutFirstrun() {}
AboutFirstrun.prototype = {
  __proto__: AboutGeneric.prototype,
  classDescription: "About Firstrun",
  contractID: "@mozilla.org/network/protocol/about;1?what=firstrun",
  classID: Components.ID("{077ea23e-0f22-4168-a744-8e444b560197}")
}

function AboutFennec() {}
AboutFennec.prototype = {
  __proto__: AboutGeneric.prototype,
  classDescription: "About Fennec",
  contractID: "@mozilla.org/network/protocol/about;1?what=fennec",
  classID: Components.ID("{842a6d11-b369-4610-ba66-c3b5217e82be}")
}

function AboutFirefox() {}
AboutFirefox.prototype = {
  __proto__: AboutGeneric.prototype,
  classDescription: "About Firefox",
  contractID: "@mozilla.org/network/protocol/about;1?what=firefox",
  classID: Components.ID("{dd40c467-d206-4f22-9215-8fcc74c74e38}")  
}

function AboutRights() {}
AboutRights.prototype = {
  __proto__: AboutGeneric.prototype,
  classDescription: "About Rights",
  contractID: "@mozilla.org/network/protocol/about;1?what=rights",
  classID: Components.ID("{3b988fbf-ec97-4e1c-a5e4-573d999edc9c}")
}

function AboutCertError() {}
AboutCertError.prototype = {
  __proto__: AboutGeneric.prototype,
  classDescription: "About Certificate Error",
  contractID: "@mozilla.org/network/protocol/about;1?what=certerror",
  classID: Components.ID("{972efe64-8ac0-4e91-bdb0-22835d987815}")
}

function AboutHome() {}
AboutHome.prototype = {
  __proto__: AboutGeneric.prototype,
  classDescription: "About Home",
  contractID: "@mozilla.org/network/protocol/about;1?what=home",
  classID: Components.ID("{b071364f-ab68-4669-a9db-33fca168271a}")
}

function AboutSyncTabs() {}
AboutSyncTabs.prototype = {
  __proto__: AboutGeneric.prototype,
  classDescription: "About Sync Tabs",
  contractID: "@mozilla.org/network/protocol/about;1?what=sync-tabs",
  classID: Components.ID("{d503134a-f6f3-4824-bc3c-09c123177944}")
}


function NSGetModule(compMgr, fileSpec)
  XPCOMUtils.generateModule([AboutFirstrun, AboutFennec, AboutRights,
                             AboutCertError, AboutFirefox, AboutHome,
                             AboutSyncTabs]);
