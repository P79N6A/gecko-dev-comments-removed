


"use strict";

Cu.import("resource://gre/modules/FxAccounts.jsm");
Cu.import("resource://gre/modules/FxAccountsClient.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://gre/modules/osfile.jsm");


let {AccountState} = Cu.import("resource://gre/modules/FxAccounts.jsm", {});

function promiseNotification(topic) {
  return new Promise(resolve => {
    let observe = () => {
      Services.obs.removeObserver(observe, topic);
      resolve();
    }
    Services.obs.addObserver(observe, topic, false);
  });
}


function MockStorageManager() {
}

MockStorageManager.prototype = {
  promiseInitialized: Promise.resolve(),

  initialize(accountData) {
    this.accountData = accountData;
  },

  finalize() {
    return Promise.resolve();
  },

  getAccountData() {
    return Promise.resolve(this.accountData);
  },

  updateAccountData(updatedFields) {
    for (let [name, value] of Iterator(updatedFields)) {
      if (value == null) {
        delete this.accountData[name];
      } else {
        this.accountData[name] = value;
      }
    }
    return Promise.resolve();
  },

  deleteAccountData() {
    this.accountData = null;
    return Promise.resolve();
  }
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
    newAccountState(credentials) {
      
      let storage = new MockStorageManager();
      storage.initialize(credentials);
      return new AccountState(this, storage);
    },
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

  deepEqual(cas.oauthTokens, {"bar|foo": tokenData});
  
  yield promiseWritten;

  
  deepEqual(cas.storageManager.accountData.oauthTokens, {"bar|foo": tokenData});

  
  promiseWritten = promiseNotification("testhelper-fxa-cache-persist-done");
  yield cas.removeCachedToken("token1");
  deepEqual(cas.oauthTokens, {});
  yield promiseWritten;
  deepEqual(cas.storageManager.accountData.oauthTokens, {});

  
  let storageManager = cas.storageManager; 
  yield fxa.signOut(  true);
  deepEqual(storageManager.accountData, null);
});
