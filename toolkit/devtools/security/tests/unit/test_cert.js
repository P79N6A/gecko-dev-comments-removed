


"use strict";

const { utils: Cu, classes: Cc, interfaces: Ci } = Components;

const { Promise: promise } =
  Cu.import("resource://gre/modules/Promise.jsm", {});
const certService = Cc["@mozilla.org/security/local-cert-service;1"]
                    .getService(Ci.nsILocalCertService);

const gNickname = "devtools";

function run_test() {
  
  do_get_profile();
  
  Cc["@mozilla.org/psm;1"].getService(Ci.nsISupports);
  run_next_test();
}

function getOrCreateCert(nickname) {
  let deferred = promise.defer();
  certService.getOrCreateCert(nickname, {
    handleCert: function(c, rv) {
      if (rv) {
        deferred.reject(rv);
        return;
      }
      deferred.resolve(c);
    }
  });
  return deferred.promise;
}

function removeCert(nickname) {
  let deferred = promise.defer();
  certService.removeCert(nickname, {
    handleResult: function(rv) {
      if (rv) {
        deferred.reject(rv);
        return;
      }
      deferred.resolve();
    }
  });
  return deferred.promise;
}

add_task(function*() {
  
  ok(!certService.loginPromptRequired);

  let certA = yield getOrCreateCert(gNickname);
  equal(certA.nickname, gNickname);

  
  let certB = yield getOrCreateCert(gNickname);
  equal(certB.nickname, gNickname);

  
  ok(certA.equals(certB));

  
  ok(certA.isSelfSigned);
  equal(certA.certType, Ci.nsIX509Cert.USER_CERT);

  
  let diffNameCert = yield getOrCreateCert("cool-stuff");
  ok(!diffNameCert.equals(certA));

  
  yield removeCert(gNickname);
  let newCert = yield getOrCreateCert(gNickname);
  ok(!newCert.equals(certA));

  
  let serial = newCert.serialNumber;
  certA = certB = diffNameCert = newCert = null;
  Cu.forceGC();
  Cu.forceCC();

  
  let certAfterGC = yield getOrCreateCert(gNickname);
  equal(certAfterGC.serialNumber, serial);
});
