


"use strict";

Cu.import("resource://gre/modules/FxAccounts.jsm");
Cu.import("resource://gre/modules/FxAccountsClient.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://gre/modules/osfile.jsm");

function promiseNotification(topic) {
  return new Promise(resolve => {
    let observe = () => {
      Services.obs.removeObserver(observe, topic);
      resolve();
    }
    Services.obs.addObserver(observe, topic, false);
  });
}


function MockFxAccountsClient() {
  this._email = "nobody@example.com";
  this._verified = false;

  this.accountStatus = function(uid) {
    let deferred = Promise.defer();
    deferred.resolve(!!uid && (!this._deletedOnServer));
    return deferred.promise;
  };

  this.signOut = function() { return Promise.resolve(); };

  FxAccountsClient.apply(this);
}

MockFxAccountsClient.prototype = {
  __proto__: FxAccountsClient.prototype
}

function MockFxAccounts() {
  return new FxAccounts({
    fxAccountsClient: new MockFxAccountsClient(),
  });
}

function* createMockFxA() {
  let fxa = new MockFxAccounts();
  let credentials = {
    email: "foo@example.com",
    uid: "1234@lcip.org",
    assertion: "foobar",
    sessionToken: "dead",
    kA: "beef",
    kB: "cafe",
    verified: true
  };
  yield fxa.setSignedInUser(credentials);
  return fxa;
}


function run_test() {
  run_next_test();
}

add_task(function testCacheStorage() {
  let fxa = yield createMockFxA();

  
  let cas = fxa.internal.currentAccountState;
  let origPersistCached = cas._persistCachedTokens.bind(cas)
  cas._persistCachedTokens = function() {
    return origPersistCached().then(() => {
      Services.obs.notifyObservers(null, "testhelper-fxa-cache-persist-done", null);
    });
  };

  let promiseWritten = promiseNotification("testhelper-fxa-cache-persist-done");
  let tokenData = {token: "token1", somethingelse: "something else"};
  let scopeArray = ["foo", "bar"];
  cas.setCachedToken(scopeArray, tokenData);
  deepEqual(cas.getCachedToken(scopeArray), tokenData);

  deepEqual(cas.getAllCachedTokens(), [tokenData]);
  
  yield promiseWritten;

  
  let path = OS.Path.join(OS.Constants.Path.profileDir, DEFAULT_OAUTH_TOKENS_FILENAME);
  let data = yield CommonUtils.readJSON(path);
  ok(data.tokens, "the data is in the json");
  equal(data.uid, "1234@lcip.org", "The user's uid is in the json");

  
  let expectedKey = "bar|foo";
  let entry = data.tokens[expectedKey];
  ok(entry, "our key is in the json");
  deepEqual(entry, tokenData, "correct token is in the json");

  
  promiseWritten = promiseNotification("testhelper-fxa-cache-persist-done");
  yield cas.removeCachedToken("token1");
  deepEqual(cas.getAllCachedTokens(), []);
  yield promiseWritten;
  data = yield CommonUtils.readJSON(path);
  ok(!data.tokens[expectedKey], "our key was removed from the json");

  
  yield fxa.signOut(  true);
  data = yield CommonUtils.readJSON(path);
  ok(data === null, "data wiped on signout");
});


add_task(function testCacheAfterRead() {
  let fxa = yield createMockFxA();
  
  let cas = fxa.internal.currentAccountState;
  let origPersistCached = cas._persistCachedTokens.bind(cas)
  cas._persistCachedTokens = function() {
    return origPersistCached().then(() => {
      Services.obs.notifyObservers(null, "testhelper-fxa-cache-persist-done", null);
    });
  };

  let promiseWritten = promiseNotification("testhelper-fxa-cache-persist-done");
  let tokenData = {token: "token1", somethingelse: "something else"};
  let scopeArray = ["foo", "bar"];
  cas.setCachedToken(scopeArray, tokenData);
  yield promiseWritten;

  
  cas.signedInUser = null;
  cas.oauthTokens = null;
  yield cas.getUserAccountData();
  ok(cas.oauthTokens, "token data was re-read");
  deepEqual(cas.getCachedToken(scopeArray), tokenData);
});


add_task(function testCacheAfterRead() {
  let fxa = yield createMockFxA();
  
  let cas = fxa.internal.currentAccountState;
  let origPersistCached = cas._persistCachedTokens.bind(cas)

  
  
  
  cas.signedInUser = null;
  cas.oauthTokens = null;

  yield cas.getUserAccountData();

  
  cas._persistCachedTokens = function() {
    return origPersistCached().then(() => {
      Services.obs.notifyObservers(null, "testhelper-fxa-cache-persist-done", null);
    });
  };
  let promiseWritten = promiseNotification("testhelper-fxa-cache-persist-done");

  
  let tokenData = {token: "token1", somethingelse: "something else"};
  let scopeArray = ["foo", "bar"];
  cas.setCachedToken(scopeArray, tokenData);

  yield promiseWritten;

  
  let got = yield cas.signedInUserStorage.getOAuthTokens();
  ok(got, "got persisted data");
  ok(got.tokens, "have tokens");
  
  ok(got.tokens["bar|foo"], "have our scope");
  equal(got.tokens["bar|foo"].token, "token1", "have our token");
});


add_task(function testCacheAfterReadBadUID() {
  let fxa = yield createMockFxA();
  
  let cas = fxa.internal.currentAccountState;
  let origPersistCached = cas._persistCachedTokens.bind(cas)
  cas._persistCachedTokens = function() {
    return origPersistCached().then(() => {
      Services.obs.notifyObservers(null, "testhelper-fxa-cache-persist-done", null);
    });
  };

  let promiseWritten = promiseNotification("testhelper-fxa-cache-persist-done");
  let tokenData = {token: "token1", somethingelse: "something else"};
  let scopeArray = ["foo", "bar"];
  cas.setCachedToken(scopeArray, tokenData);
  yield promiseWritten;

  
  cas.signedInUser = null;
  cas.oauthTokens = null;

  
  let path = OS.Path.join(OS.Constants.Path.profileDir, DEFAULT_OAUTH_TOKENS_FILENAME);
  let data = yield CommonUtils.readJSON(path);
  ok(data.tokens, "the data is in the json");
  equal(data.uid, "1234@lcip.org", "The user's uid is in the json");
  data.uid = "someone_else";
  yield CommonUtils.writeJSON(data, path);

  yield cas.getUserAccountData();
  deepEqual(cas.oauthTokens, {}, "token data ignored due to bad uid");
  equal(null, cas.getCachedToken(scopeArray), "no token available");
});
