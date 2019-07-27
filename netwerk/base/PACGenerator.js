



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.import('resource://gre/modules/Services.jsm');

let DEBUG = false;

if (DEBUG) {
  debug = function (s) { dump("-*- PACGenerator: " + s + "\n"); };
}
else {
  debug = function (s) {};
}

const PACGENERATOR_CONTRACTID = "@mozilla.org/pac-generator;1";
const PACGENERATOR_CID = Components.ID("{788507c4-eb5f-4de8-b19b-e0d531974e8a}");










const HOST_REGEX =
  new RegExp("^(?:" +
               
               "(?:[a-z0-9](?:[a-z0-9-]*[a-z0-9])?\\.)*" +
               
               "[a-z](?:[a-z0-9-]*[a-z0-9])?" +
             "|" +
               
               "\\d+\\.\\d+\\.\\d+\\.\\d+" +
             ")$",
             "i");

function PACGenerator() {
  debug("Starting PAC Generator service.");
}

PACGenerator.prototype = {

  classID : PACGENERATOR_CID,

  QueryInterface : XPCOMUtils.generateQI([Ci.nsIPACGenerator]),

  classInfo : XPCOMUtils.generateCI({classID: PACGENERATOR_CID,
                                     contractID: PACGENERATOR_CONTRACTID,
                                     classDescription: "PACGenerator",
                                     interfaces: [Ci.nsIPACGenerator]}),

  


  isValidHost: function isValidHost(host) {
    if (!HOST_REGEX.test(host)) {
      debug("Unexpected host: '" + host + "'");
      return false;
    }
    return true;
  },

  



  generate: function generate() {
    let enabled, host, port, proxy;

    try {
      enabled = Services.prefs.getBoolPref("network.proxy.pac_generator");
    } catch (ex) {}
    if (!enabled) {
      debug("PAC Generator disabled.");
      return "";
    }

    let pac = "data:text/plain,function FindProxyForURL(url, host) { ";

    
    pac += "if (shExpMatch(host, 'localhost') || " +
           "shExpMatch(host, '127.0.0.1')) {" +
           " return 'DIRECT'; } ";

    
    try {
      enabled = Services.prefs.getBoolPref("network.proxy.browsing.enabled");
      host = Services.prefs.getCharPref("network.proxy.browsing.host");
      port = Services.prefs.getIntPref("network.proxy.browsing.port");
    } catch (ex) {}

    if (enabled && host && this.isValidHost(host)) {
      proxy = host + ":" + ((port && port !== 0) ? port : 8080);
      let appOrigins;
      try {
        appOrigins = Services.prefs.getCharPref("network.proxy.browsing.app_origins");
      } catch (ex) {}

      pac += "var origins ='" + appOrigins +
             "'.split(/[ ,]+/).filter(Boolean); " +
             "if ((origins.indexOf('*') > -1 || origins.indexOf(myAppOrigin()) > -1)" +
             " && isInBrowser()) { return 'PROXY " + proxy + "'; } ";
    }

    
    let share;
    try {
      share = Services.prefs.getBoolPref("network.proxy.share_proxy_settings");
    } catch (ex) {}

    if (share) {
      
      try {
        host = Services.prefs.getCharPref("network.proxy.http");
        port = Services.prefs.getIntPref("network.proxy.http_port");
      } catch (ex) {}

      if (host && this.isValidHost(host)) {
        proxy = host + ":" + ((port && port !== 0) ? port : 8080);
        pac += "return 'PROXY " + proxy + "'; "
      } else {
        pac += "return 'DIRECT'; ";
      }
    } else {
      
      const proxyTypes = [
        {"scheme": "http:", "pref": "http"},
        {"scheme": "https:", "pref": "ssl"},
        {"scheme": "ftp:", "pref": "ftp"}
      ];
      for (let i = 0; i < proxyTypes.length; i++) {
       try {
          host = Services.prefs.getCharPref("network.proxy." +
                                            proxyTypes[i]["pref"]);
          port = Services.prefs.getIntPref("network.proxy." +
                                            proxyTypes[i]["pref"] + "_port");
        } catch (ex) {}

        if (host && this.isValidHost(host)) {
          proxy = host + ":" + (port === 0 ? 8080 : port);
          pac += "if (url.substring(0, " + (proxyTypes[i]["scheme"]).length +
                 ") == '" + proxyTypes[i]["scheme"] + "') { return 'PROXY " +
                 proxy + "'; } ";
         }
      }
      pac += "return 'DIRECT'; ";
    }

    pac += "}";

    debug("PAC: " + pac);

    return pac;
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([PACGenerator]);
