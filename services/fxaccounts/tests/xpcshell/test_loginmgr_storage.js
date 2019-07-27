


"use strict";




Services.prefs.setCharPref("identity.fxaccounts.auth.uri", "http://localhost");

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FxAccounts.jsm");
Cu.import("resource://gre/modules/FxAccountsClient.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/FxAccountsCommon.js");

initTestLogging("Trace");

Services.prefs.setCharPref("identity.fxaccounts.loglevel", "DEBUG");

function run_test() {
  run_next_test();
}

function getLoginMgrData() {
  let logins = Services.logins.findLogins({}, FXA_PWDMGR_HOST, null, FXA_PWDMGR_REALM);
  if (logins.length == 0) {
    return null;
  }
  Assert.equal(logins.length, 1, "only 1 login available");
  return logins[0];
}

add_task(function test_simple() {
  let fxa = new FxAccounts({});

  let creds = {
    email: "test@example.com",
    sessionToken: "sessionToken",
    kA: "the kA value",
    kB: "the kB value",
    verified: true
  };
  yield fxa.setSignedInUser(creds);

  
  
  let path = OS.Path.join(OS.Constants.Path.profileDir, "signedInUser.json");
  let data = yield CommonUtils.readJSON(path);

  Assert.strictEqual(data.accountData.email, creds.email, "correct email in the clear text");
  Assert.strictEqual(data.accountData.sessionToken, creds.sessionToken, "correct sessionToken in the clear text");
  Assert.strictEqual(data.accountData.verified, creds.verified, "correct verified flag");

  Assert.ok(!("kA" in data.accountData), "kA not stored in clear text");
  Assert.ok(!("kB" in data.accountData), "kB not stored in clear text");

  let login = getLoginMgrData();
  Assert.strictEqual(login.username, creds.email, "email matches");
  let loginData = JSON.parse(login.password);
  Assert.strictEqual(loginData.version, data.version, "same version flag in both places");
  Assert.strictEqual(loginData.accountData.kA, creds.kA, "correct kA in the login mgr");
  Assert.strictEqual(loginData.accountData.kB, creds.kB, "correct kB in the login mgr");

  Assert.ok(!("email" in loginData), "email not stored in the login mgr json");
  Assert.ok(!("sessionToken" in loginData), "sessionToken not stored in the login mgr json");
  Assert.ok(!("verified" in loginData), "verified not stored in the login mgr json");

  yield fxa.signOut( true);
  Assert.strictEqual(getLoginMgrData(), null, "login mgr data deleted on logout");
});

add_task(function test_MPLocked() {
  let fxa = new FxAccounts({});

  let creds = {
    email: "test@example.com",
    sessionToken: "sessionToken",
    kA: "the kA value",
    kB: "the kB value",
    verified: true
  };

  
  fxa.internal.signedInUserStorage.__defineGetter__("_isLoggedIn", function() false);
  yield fxa.setSignedInUser(creds);

  
  
  let path = OS.Path.join(OS.Constants.Path.profileDir, "signedInUser.json");
  let data = yield CommonUtils.readJSON(path);

  Assert.strictEqual(data.accountData.email, creds.email, "correct email in the clear text");
  Assert.strictEqual(data.accountData.sessionToken, creds.sessionToken, "correct sessionToken in the clear text");
  Assert.strictEqual(data.accountData.verified, creds.verified, "correct verified flag");

  Assert.ok(!("kA" in data.accountData), "kA not stored in clear text");
  Assert.ok(!("kB" in data.accountData), "kB not stored in clear text");

  Assert.strictEqual(getLoginMgrData(), null, "login mgr data doesn't exist");
  yield fxa.signOut( true)
});

add_task(function test_migrationMPUnlocked() {
  
  
  let fxa = new FxAccounts({});

  let creds = {
    email: "test@example.com",
    sessionToken: "sessionToken",
    kA: "the kA value",
    kB: "the kB value",
    verified: true
  };
  let toWrite = {
    version: fxa.version,
    accountData: creds,
  }

  let path = OS.Path.join(OS.Constants.Path.profileDir, "signedInUser.json");
  yield CommonUtils.writeJSON(toWrite, path);

  
  let data = yield fxa.getSignedInUser();
  Assert.deepEqual(data, creds, "we got all the data back");

  
  data = yield CommonUtils.readJSON(path);

  Assert.strictEqual(data.accountData.email, creds.email, "correct email in the clear text");
  Assert.strictEqual(data.accountData.sessionToken, creds.sessionToken, "correct sessionToken in the clear text");
  Assert.strictEqual(data.accountData.verified, creds.verified, "correct verified flag");

  Assert.ok(!("kA" in data.accountData), "kA not stored in clear text");
  Assert.ok(!("kB" in data.accountData), "kB not stored in clear text");

  let login = getLoginMgrData();
  Assert.strictEqual(login.username, creds.email, "email matches");
  let loginData = JSON.parse(login.password);
  Assert.strictEqual(loginData.version, data.version, "same version flag in both places");
  Assert.strictEqual(loginData.accountData.kA, creds.kA, "correct kA in the login mgr");
  Assert.strictEqual(loginData.accountData.kB, creds.kB, "correct kB in the login mgr");

  Assert.ok(!("email" in loginData), "email not stored in the login mgr json");
  Assert.ok(!("sessionToken" in loginData), "sessionToken not stored in the login mgr json");
  Assert.ok(!("verified" in loginData), "verified not stored in the login mgr json");

  yield fxa.signOut( true);
  Assert.strictEqual(getLoginMgrData(), null, "login mgr data deleted on logout");
});

add_task(function test_migrationMPLocked() {
  
  
  let fxa = new FxAccounts({});

  let creds = {
    email: "test@example.com",
    sessionToken: "sessionToken",
    kA: "the kA value",
    kB: "the kB value",
    verified: true
  };
  let toWrite = {
    version: fxa.version,
    accountData: creds,
  }

  let path = OS.Path.join(OS.Constants.Path.profileDir, "signedInUser.json");
  yield CommonUtils.writeJSON(toWrite, path);

  
  fxa.internal.signedInUserStorage.__defineGetter__("_isLoggedIn", function() false);

  
  
  let data = yield fxa.getSignedInUser();
  Assert.ok(!data.kA);
  Assert.ok(!data.kB);

  
  data = yield CommonUtils.readJSON(path);
  Assert.deepEqual(data, toWrite);

  
  fxa.internal.signedInUserStorage.__defineGetter__("_isLoggedIn", function() true);
  data = yield fxa.getSignedInUser();
  
  Assert.strictEqual(data.kA, creds.kA);
  Assert.strictEqual(data.kB, creds.kB);

  
  data = yield CommonUtils.readJSON(path);
  Assert.strictEqual(data.accountData.email, creds.email, "correct email in the clear text");
  Assert.strictEqual(data.accountData.sessionToken, creds.sessionToken, "correct sessionToken in the clear text");
  Assert.strictEqual(data.accountData.verified, creds.verified, "correct verified flag");

  Assert.ok(!("kA" in data.accountData), "kA not stored in clear text");
  Assert.ok(!("kB" in data.accountData), "kB not stored in clear text");

  let login = getLoginMgrData();
  Assert.strictEqual(login.username, creds.email, "email matches");
  let loginData = JSON.parse(login.password);
  Assert.strictEqual(loginData.version, data.version, "same version flag in both places");
  Assert.strictEqual(loginData.accountData.kA, creds.kA, "correct kA in the login mgr");
  Assert.strictEqual(loginData.accountData.kB, creds.kB, "correct kB in the login mgr");

  Assert.ok(!("email" in loginData), "email not stored in the login mgr json");
  Assert.ok(!("sessionToken" in loginData), "sessionToken not stored in the login mgr json");
  Assert.ok(!("verified" in loginData), "verified not stored in the login mgr json");

  yield fxa.signOut( true);
  Assert.strictEqual(getLoginMgrData(), null, "login mgr data deleted on logout");
});

add_task(function test_consistentWithMPEdgeCases() {
  let fxa = new FxAccounts({});

  let creds1 = {
    email: "test@example.com",
    sessionToken: "sessionToken",
    kA: "the kA value",
    kB: "the kB value",
    verified: true
  };

  let creds2 = {
    email: "test2@example.com",
    sessionToken: "sessionToken2",
    kA: "the kA value2",
    kB: "the kB value2",
    verified: false,
  };

  
  yield fxa.setSignedInUser(creds1);

  
  
  fxa.internal.signedInUserStorage.__defineGetter__("_isLoggedIn", function() false);

  
  yield fxa.setSignedInUser(creds2);

  
  let login = getLoginMgrData();
  Assert.strictEqual(login.username, creds1.email);
  
  Assert.strictEqual(JSON.parse(login.password).accountData.kA, creds1.kA,
                     "stale data still in login mgr");

  
  
  
  fxa = new FxAccounts({});

  let accountData = yield fxa.getSignedInUser();
  Assert.strictEqual(accountData.email, creds2.email);
  
  Assert.strictEqual(accountData.kA, undefined, "stale kA wasn't used");
  yield fxa.signOut( true)
});

add_task(function test_migration() {
  
  
  let creds = {
    email: "test@example.com",
    sessionToken: "sessionToken",
    kA: "the kA value",
    kB: "the kB value",
    verified: true
  };
  let toWrite = {
    version: 1,
    accountData: creds,
  };

  let path = OS.Path.join(OS.Constants.Path.profileDir, "signedInUser.json");
  let data = yield CommonUtils.writeJSON(toWrite, path);

  
  let fxa = new FxAccounts({});
  data = yield fxa.getSignedInUser();

  Assert.deepEqual(data, creds, "we should have everything available");

  
  data = yield CommonUtils.readJSON(path);

  Assert.strictEqual(data.accountData.email, creds.email, "correct email in the clear text");
  Assert.strictEqual(data.accountData.sessionToken, creds.sessionToken, "correct sessionToken in the clear text");
  Assert.strictEqual(data.accountData.verified, creds.verified, "correct verified flag");

  Assert.ok(!("kA" in data.accountData), "kA not stored in clear text");
  Assert.ok(!("kB" in data.accountData), "kB not stored in clear text");

  
  let login = getLoginMgrData();
  Assert.strictEqual(login.username, creds.email);
  
  Assert.strictEqual(JSON.parse(login.password).accountData.kA, creds.kA,
                     "kA was migrated");

  yield fxa.signOut( true)
});
