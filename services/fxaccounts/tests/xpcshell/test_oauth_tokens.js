


"use strict";

Cu.import("resource://gre/modules/FxAccounts.jsm");
Cu.import("resource://gre/modules/FxAccountsClient.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://gre/modules/FxAccountsOAuthGrantClient.jsm");
Cu.import("resource://services-common/utils.js");

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

function MockFxAccounts(mockGrantClient) {
  return new FxAccounts({
    fxAccountsClient: new MockFxAccountsClient(),
    getAssertion: () => Promise.resolve("assertion"),
    _destroyOAuthToken: function(tokenData) {
      
      return mockGrantClient.destroyToken(tokenData.token).then( () => {
        Services.obs.notifyObservers(null, "testhelper-fxa-revoke-complete", null);
      });
    },
  });
}

function* createMockFxA(mockGrantClient) {
  let fxa = new MockFxAccounts(mockGrantClient);
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

function MockFxAccountsOAuthGrantClient() {
  this.activeTokens = new Set();
}

MockFxAccountsOAuthGrantClient.prototype = {
  serverURL: {href: "http://localhost"},
  getTokenFromAssertion(assertion, scope) {
    let token = "token" + this.numTokenFetches;
    this.numTokenFetches += 1;
    this.activeTokens.add(token);
    print("getTokenFromAssertion returning token", token);
    return Promise.resolve({access_token: token});
  },
  destroyToken(token) {
    ok(this.activeTokens.delete(token));
    print("after destroy have", this.activeTokens.size, "tokens left.");
    return Promise.resolve({});
  },
  
  numTokenFetches: 0,
  activeTokens: null,
}

add_task(function testRevoke() {
  let client = new MockFxAccountsOAuthGrantClient();
  let tokenOptions = { scope: "test-scope", client: client };
  let fxa = yield createMockFxA(client);

  
  let token1 = yield fxa.getOAuthToken(tokenOptions);
  equal(client.numTokenFetches, 1);
  equal(client.activeTokens.size, 1);
  ok(token1, "got a token");
  equal(token1, "token0");

  
  yield fxa.removeCachedOAuthToken({token: token1});

  
  yield promiseNotification("testhelper-fxa-revoke-complete");
  
  equal(client.activeTokens.size, 0);
  
  let token2 = yield fxa.getOAuthToken(tokenOptions);
  equal(client.numTokenFetches, 2);
  equal(client.activeTokens.size, 1);
  ok(token2, "got a token");
  notEqual(token1, token2, "got a different token");
});

add_task(function testSignOutDestroysTokens() {
  let client = new MockFxAccountsOAuthGrantClient();
  let fxa = yield createMockFxA(client);

  
  let token1 = yield fxa.getOAuthToken({ scope: "test-scope", client: client });
  equal(client.numTokenFetches, 1);
  equal(client.activeTokens.size, 1);
  ok(token1, "got a token");

  
  let token2 = yield fxa.getOAuthToken({ scope: "test-scope-2", client: client });
  equal(client.numTokenFetches, 2);
  equal(client.activeTokens.size, 2);
  ok(token2, "got a token");
  notEqual(token1, token2, "got a different token");

  
  yield fxa.signOut();
  
  yield promiseNotification("testhelper-fxa-signout-complete");
  
  equal(client.activeTokens.size, 0);
});

add_task(function testTokenRaces() {
  
  
  
  
  
  let client = new MockFxAccountsOAuthGrantClient();
  let fxa = yield createMockFxA(client);

  
  
  let notifications = Promise.all([
    promiseNotification("testhelper-fxa-revoke-complete"),
    promiseNotification("testhelper-fxa-revoke-complete"),
  ]);
  let results = yield Promise.all([
    fxa.getOAuthToken({scope: "test-scope", client: client}),
    fxa.getOAuthToken({scope: "test-scope", client: client}),
    fxa.getOAuthToken({scope: "test-scope-2", client: client}),
    fxa.getOAuthToken({scope: "test-scope-2", client: client}),
  ]);

  equal(client.numTokenFetches, 4, "should have fetched 4 tokens.");
  
  yield notifications;

  
  results.sort();
  equal(results[0], results[1]);
  equal(results[2], results[3]);
  
  equal(client.activeTokens.size, 2);
  
  notifications = Promise.all([
    promiseNotification("testhelper-fxa-revoke-complete"),
    promiseNotification("testhelper-fxa-revoke-complete"),
  ]);
  yield fxa.removeCachedOAuthToken({token: results[0]});
  equal(client.activeTokens.size, 1);
  yield fxa.removeCachedOAuthToken({token: results[2]});
  equal(client.activeTokens.size, 0);
  yield notifications;
});
