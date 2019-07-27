





"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

this.EXPORTED_SYMBOLS = ["TrustedHostedAppsUtils"];

Cu.import("resource://gre/modules/Services.jsm");

#ifdef MOZ_WIDGET_ANDROID




let debug = Cu
  .import("resource://gre/modules/AndroidLog.jsm", {})
  .AndroidLog.d.bind(null, "TrustedHostedAppsUtils");
#else


let debug = Services.prefs.getBoolPref("dom.mozApps.debug") ?
  aMsg => dump("-*- TrustedHostedAppsUtils.jsm : " + aMsg + "\n") :
  () => {};
#endif






this.TrustedHostedAppsUtils = {

  


  isHostPinned: function (aUrl) {
    let uri;
    try {
      uri = Services.io.newURI(aUrl, null, null);
    } catch(e) {
      debug("Host parsing failed: " + e);
      return false;
    }

    
    if (!uri.host || "https" != uri.scheme) {
      return false;
    }

    
    let siteSecurityService;
    try {
      siteSecurityService = Cc["@mozilla.org/ssservice;1"]
        .getService(Ci.nsISiteSecurityService);
    } catch (e) {
      debug("nsISiteSecurityService error: " + e);
      
      throw "CERTDB_ERROR";
    }

    if (siteSecurityService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HPKP, uri.host, 0)) {
      debug("\tvalid certificate pinning for host: " + uri.host + "\n");
      return true;
    }

    debug("\tHost NOT pinned: " + uri.host + "\n");
    return false;
  },

  








  getCSPWhiteList: function(aCsp) {
    let isValid = false;
    let whiteList = [];
    let requiredDirectives = [ "script-src", "style-src" ];

    if (aCsp) {
      let validDirectives = [];
      let directives = aCsp.split(";");
      
      directives
        .map(aDirective => aDirective.trim().split(" "))
        .filter(aList => aList.length > 1)
        
        .filter(aList => (requiredDirectives.indexOf(aList[0]) != -1))
        .forEach(aList => {
          
          
          let directiveName = aList.shift()
          let sources = aList;

          if ((-1 == validDirectives.indexOf(directiveName))) {
            validDirectives.push(directiveName);
          }
          whiteList.push(...sources.filter(
             
            aSource => (aSource !="'self'" && whiteList.indexOf(aSource) == -1)
          ));
        });

      
      isValid = requiredDirectives.length === validDirectives.length;

      if (!isValid) {
        debug("White list doesn't contain all required directives!");
        whiteList = [];
      }
    }

    debug("White list contains " + whiteList.length + " hosts");
    return { list: whiteList, valid: isValid };
  },

  





  verifyCSPWhiteList: function(aCsp) {
    let domainWhitelist = this.getCSPWhiteList(aCsp);
    if (!domainWhitelist.valid) {
      debug("TRUSTED_APPLICATION_WHITELIST_PARSING_FAILED");
      return false;
    }

    if (!domainWhitelist.list.every(aUrl => this.isHostPinned(aUrl))) {
      debug("TRUSTED_APPLICATION_WHITELIST_VALIDATION_FAILED");
      return false;
    }

    return true;
  }
};
