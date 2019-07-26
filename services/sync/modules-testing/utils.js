



"use strict";

this.EXPORTED_SYMBOLS = [
  "TEST_CLUSTER_URL",
  "TEST_SERVER_URL",
  "btoa", 
  "encryptPayload",
  "setBasicCredentials",
  "SyncTestingInfrastructure",
  "waitForZeroTimer",
];

const {utils: Cu} = Components;

Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-crypto/utils.js");
Cu.import("resource://testing-common/services-common/logging.js");
Cu.import("resource://testing-common/services/sync/fakeservices.js");

this.TEST_SERVER_URL = "http://localhost:8080/";
this.TEST_CLUSTER_URL = TEST_SERVER_URL;






this.waitForZeroTimer = function waitForZeroTimer(callback) {
  let ticks = 2;
  function wait() {
    if (ticks) {
      ticks -= 1;
      CommonUtils.nextTick(wait);
      return;
    }
    callback();
  }
  CommonUtils.namedTimer(wait, 150, {}, "timer");
}

this.setBasicCredentials =
 function setBasicCredentials(username, password, syncKey) {
  let ns = {};
  Cu.import("resource://services-sync/service.js", ns);

  let auth = ns.Service.identity;
  auth.username = username;
  auth.basicPassword = password;
  auth.syncKey = syncKey;
}

this.SyncTestingInfrastructure =
 function SyncTestingInfrastructure(username, password, syncKey) {
  let ns = {};
  Cu.import("resource://services-sync/service.js", ns);

  let auth = ns.Service.identity;
  auth.account = username || "foo";
  auth.basicPassword = password || "password";
  auth.syncKey = syncKey || "foo";

  ns.Service.serverURL = TEST_SERVER_URL;
  ns.Service.clusterURL = TEST_CLUSTER_URL;

  this.logStats = initTestLogging();
  this.fakeFilesystem = new FakeFilesystemService({});
  this.fakeGUIDService = new FakeGUIDService();
  this.fakeCryptoService = new FakeCryptoService();
}




this.encryptPayload = function encryptPayload(cleartext) {
  if (typeof cleartext == "object") {
    cleartext = JSON.stringify(cleartext);
  }

  return {
    ciphertext: cleartext, 
    IV: "irrelevant",
    hmac: fakeSHA256HMAC(cleartext, CryptoUtils.makeHMACKey("")),
  };
}

