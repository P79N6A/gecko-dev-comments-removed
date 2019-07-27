





"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const signatureFileExtension = ".sig";

this.EXPORTED_SYMBOLS = ["TrustedHostedAppsUtils"];

Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
  "resource://gre/modules/NetUtil.jsm");

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

    if (siteSecurityService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HPKP,
                                         uri.host, 0)) {
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
          
          
          let directiveName = aList.shift();
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
  },

  _verifySignedFile: function(aManifestStream, aSignatureStream, aCertDb) {
    let deferred = Promise.defer();

    let root = Ci.nsIX509CertDB.TrustedHostedAppPublicRoot;
    try {
      
      
      
      
      let useTrustedAppTestCerts = Services.prefs
        .getBoolPref("dom.mozApps.use_trustedapp_test_certs");
      if (useTrustedAppTestCerts) {
        root = Ci.nsIX509CertDB.TrustedHostedAppTestRoot;
      }
    } catch (ex) { }

    aCertDb.verifySignedManifestAsync(
      root, aManifestStream, aSignatureStream,
      function(aRv, aCert) {
        debug("Signature verification returned code, cert & root: " + aRv + " " + aCert + " " + root);
        if (Components.isSuccessCode(aRv)) {
          deferred.resolve(aCert);
        } else if (aRv == Cr.NS_ERROR_FILE_CORRUPTED ||
                   aRv == Cr.NS_ERROR_SIGNED_MANIFEST_FILE_INVALID) {
          deferred.reject("MANIFEST_SIGNATURE_FILE_INVALID");
        } else {
          deferred.reject("MANIFEST_SIGNATURE_VERIFICATION_ERROR");
        }
      }
    );

    return deferred.promise;
  },

  verifySignedManifest: function(aApp, aAppId) {
    let deferred = Promise.defer();

    let certDb;
    try {
      certDb = Cc["@mozilla.org/security/x509certdb;1"]
                 .getService(Ci.nsIX509CertDB);
    } catch (e) {
      debug("nsIX509CertDB error: " + e);
      
      throw "CERTDB_ERROR";
    }

    let mRequestChannel = NetUtil.newChannel(aApp.manifestURL)
                                 .QueryInterface(Ci.nsIHttpChannel);
    mRequestChannel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
    mRequestChannel.notificationCallbacks =
      AppsUtils.createLoadContext(aAppId, false);

    
    
    
    let signatureURL;
    try {
      let mURL = Cc["@mozilla.org/network/io-service;1"]
        .getService(Ci.nsIIOService)
        .newURI(aApp.manifestURL, null, null)
        .QueryInterface(Ci.nsIURL);
      signatureURL = mURL.prePath +
        mURL.directory + mURL.fileBaseName + signatureFileExtension;
    } catch(e) {
      deferred.reject("SIGNATURE_PATH_INVALID");
      return;
    }

    let sRequestChannel = NetUtil.newChannel(signatureURL)
      .QueryInterface(Ci.nsIHttpChannel);
    sRequestChannel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
    sRequestChannel.notificationCallbacks =
      AppsUtils.createLoadContext(aAppId, false);
    let getAsyncFetchCallback = (resolve, reject) =>
        (aInputStream, aResult) => {
          if (!Components.isSuccessCode(aResult)) {
            debug("Failed to download file");
            reject("MANIFEST_FILE_UNAVAILABLE");
            return;
          }
          resolve(aInputStream);
        };

    Promise.all([
      new Promise((resolve, reject) => {
        NetUtil.asyncFetch(mRequestChannel,
                           getAsyncFetchCallback(resolve, reject));
      }),
      new Promise((resolve, reject) => {
        NetUtil.asyncFetch(sRequestChannel,
                           getAsyncFetchCallback(resolve, reject));
      })
    ]).then(([aManifestStream, aSignatureStream]) => {
      this._verifySignedFile(aManifestStream, aSignatureStream, certDb)
        .then(deferred.resolve, deferred.reject);
    }, deferred.reject);

    return deferred.promise;
  },

  verifyManifest: function(aData) {
    return new Promise((resolve, reject) => {
      
      
      if (!this.isHostPinned(aData.app.manifestURL)) {
        reject("TRUSTED_APPLICATION_HOST_CERTIFICATE_INVALID");
        return;
      }
      if (!this.verifyCSPWhiteList(aData.app.manifest.csp)) {
        reject("TRUSTED_APPLICATION_WHITELIST_VALIDATION_FAILED");
        return;
      }
      this.verifySignedManifest(aData.app, aData.appId).then(resolve, reject);
    });
  }
};
