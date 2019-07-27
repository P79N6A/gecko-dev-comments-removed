





"use strict";

let { Ci, Cc } = require("chrome");
let promise = require("promise");
let DevToolsUtils = require("devtools/toolkit/DevToolsUtils");
DevToolsUtils.defineLazyGetter(this, "localCertService", () => {
  
  Cc["@mozilla.org/psm;1"].getService(Ci.nsISupports);
  return Cc["@mozilla.org/security/local-cert-service;1"]
         .getService(Ci.nsILocalCertService);
});

const localCertName = "devtools";

exports.local = {

  









  getOrCreate() {
    let deferred = promise.defer();
    localCertService.getOrCreateCert(localCertName, {
      handleCert: function(cert, rv) {
        if (rv) {
          deferred.reject(rv);
          return;
        }
        deferred.resolve(cert);
      }
    });
    return deferred.promise;
  },

  




  remove() {
    let deferred = promise.defer();
    localCertService.removeCert(localCertName, {
      handleCert: function(rv) {
        if (rv) {
          deferred.reject(rv);
          return;
        }
        deferred.resolve();
      }
    });
    return deferred.promise;
  }

};
