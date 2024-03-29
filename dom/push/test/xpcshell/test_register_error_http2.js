


'use strict';

Cu.import("resource://gre/modules/Services.jsm");

const {PushDB, PushService, PushServiceHttp2} = serviceExports;

var prefs;
var tlsProfile;
var serverURL;

var serverPort = -1;

function run_test() {
  var env = Cc["@mozilla.org/process/environment;1"].getService(Ci.nsIEnvironment);
  serverPort = env.get("MOZHTTP2-PORT");
  do_check_neq(serverPort, null);

  do_get_profile();
  prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);

  tlsProfile = prefs.getBoolPref("network.http.spdy.enforce-tls-profile");

  disableServiceWorkerEvents(
    'https://example.net/page/invalid-response'
  );

  serverURL = "https://localhost:" + serverPort;

  run_next_test();
}


add_task(function* test_pushSubscriptionNoConnection() {

  let db = PushServiceHttp2.newPushDB();
  do_register_cleanup(() => {
    return db.drop().then(_ => db.close());
  });

  PushService.init({
    serverURI: serverURL + "/pushSubscriptionNoConnection/subscribe",
    db
  });

  yield rejects(
    PushNotificationService.register(
      'https://example.net/page/invalid-response',
      ChromeUtils.originAttributesToSuffix({ appId: Ci.nsIScriptSecurityManager.NO_APP_ID, inBrowser: false })),
    function(error) {
      return error && error.includes("Error");
    },
    'Wrong error for not being able to establish connecion.'
  );

  let record = yield db.getAllKeyIDs();
  ok(record.length === 0, "Should not store records when connection couldn't be established.");
  PushService.uninit();
});

add_task(function* test_TLS() {
    
  var oldPref = prefs.getIntPref("network.http.speculative-parallel-limit");
  prefs.setIntPref("network.http.speculative-parallel-limit", 0);
  prefs.setBoolPref("network.http.spdy.enforce-tls-profile", false);

  addCertOverride("localhost", serverPort,
                  Ci.nsICertOverrideService.ERROR_UNTRUSTED |
                  Ci.nsICertOverrideService.ERROR_MISMATCH |
                  Ci.nsICertOverrideService.ERROR_TIME);

  prefs.setIntPref("network.http.speculative-parallel-limit", oldPref);
});

add_task(function* test_pushSubscriptionMissingLocation() {

  let db = PushServiceHttp2.newPushDB();
  do_register_cleanup(() => {
    return db.drop().then(_ => db.close());
  });

  PushService.init({
    serverURI: serverURL + "/pushSubscriptionMissingLocation/subscribe",
    db
  });

  yield rejects(
    PushNotificationService.register(
      'https://example.net/page/invalid-response',
      ChromeUtils.originAttributesToSuffix({ appId: Ci.nsIScriptSecurityManager.NO_APP_ID, inBrowser: false })),
    function(error) {
      return error && error.includes("Return code 201, but the answer is bogus");
    },
    'Wrong error for the missing location header.'
  );

  let record = yield db.getAllKeyIDs();
  ok(record.length === 0, 'Should not store records when the location header is missing.');
  PushService.uninit();
});

add_task(function* test_pushSubscriptionMissingLink() {

  let db = PushServiceHttp2.newPushDB();
  do_register_cleanup(() => {
    return db.drop().then(_ => db.close());
  });

  PushService.init({
    serverURI: serverURL + "/pushSubscriptionMissingLink/subscribe",
    db
  });

  yield rejects(
    PushNotificationService.register(
      'https://example.net/page/invalid-response',
      ChromeUtils.originAttributesToSuffix({ appId: Ci.nsIScriptSecurityManager.NO_APP_ID, inBrowser: false })),
    function(error) {
      return error && error.includes("Return code 201, but the answer is bogus");
    },
    'Wrong error for the missing link header.'
  );

  let record = yield db.getAllKeyIDs();
  ok(record.length === 0, 'Should not store records when a link header is missing.');
  PushService.uninit();
});

add_task(function* test_pushSubscriptionMissingLink1() {

  let db = PushServiceHttp2.newPushDB();
  do_register_cleanup(() => {
    return db.drop().then(_ => db.close());
  });

  PushService.init({
    serverURI: serverURL + "/pushSubscriptionMissingLink1/subscribe",
    db
  });

  yield rejects(
    PushNotificationService.register(
      'https://example.net/page/invalid-response',
      ChromeUtils.originAttributesToSuffix({ appId: Ci.nsIScriptSecurityManager.NO_APP_ID, inBrowser: false })),
    function(error) {
      return error && error.includes("Return code 201, but the answer is bogus");
    },
    'Wrong error for the missing push endpoint.'
  );

  let record = yield db.getAllKeyIDs();
  ok(record.length === 0, 'Should not store records when the push endpoint is missing.');
  PushService.uninit();
});

add_task(function* test_pushSubscriptionLocationBogus() {

  let db = PushServiceHttp2.newPushDB();
  do_register_cleanup(() => {
    return db.drop().then(_ => db.close());
  });

  PushService.init({
    serverURI: serverURL + "/pushSubscriptionLocationBogus/subscribe",
    db
  });

  yield rejects(
    PushNotificationService.register(
      'https://example.net/page/invalid-response',
      ChromeUtils.originAttributesToSuffix({ appId: Ci.nsIScriptSecurityManager.NO_APP_ID, inBrowser: false })),
    function(error) {
      return error && error.includes("Return code 201, but URI is bogus.");
    },
    'Wrong error for the bogus location'
  );

  let record = yield db.getAllKeyIDs();
  ok(record.length === 0, 'Should not store records when location header is bogus.');
  PushService.uninit();
});

add_task(function* test_pushSubscriptionNot2xxCode() {

  let db = PushServiceHttp2.newPushDB();
  do_register_cleanup(() => {
    return db.drop().then(_ => db.close());
  });

  PushService.init({
    serverURI: serverURL + "/pushSubscriptionNot201Code/subscribe",
    db
  });

  yield rejects(
    PushNotificationService.register(
      'https://example.net/page/invalid-response',
      ChromeUtils.originAttributesToSuffix({ appId: Ci.nsIScriptSecurityManager.NO_APP_ID, inBrowser: false })),
    function(error) {
      return error && error.includes("Error");
    },
    'Wrong error for not 201 responce code.'
  );

  let record = yield db.getAllKeyIDs();
  ok(record.length === 0, 'Should not store records when respons code is not 201.');
});

add_task(function* test_complete() {
  prefs.setBoolPref("network.http.spdy.enforce-tls-profile", tlsProfile);
});
