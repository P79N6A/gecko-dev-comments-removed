


"use strict";



Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FxAccountsStorage.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://gre/modules/Log.jsm");

initTestLogging("Trace");
log.level = Log.Level.Trace;


function MockedPlainStorage(accountData) {
  let data = null;
  if (accountData) {
    data = {
      version: DATA_FORMAT_VERSION,
      accountData: accountData,
    }
  }
  this.data = data;
  this.numReads = 0;
}
MockedPlainStorage.prototype = {
  get: Task.async(function* () {
    this.numReads++;
    Assert.equal(this.numReads, 1, "should only ever be 1 read of acct data");
    return this.data;
  }),

  set: Task.async(function* (data) {
    this.data = data;
  }),
};

function MockedSecureStorage(accountData) {
  let data = null;
  if (accountData) {
    data = {
      version: DATA_FORMAT_VERSION,
      accountData: accountData,
    }
  }
  this.data = data;
  this.numReads = 0;
}

MockedSecureStorage.prototype = {
  locked: false,
  STORAGE_LOCKED: function() {},
  get: Task.async(function* (uid, email) {
    if (this.locked) {
      throw new this.STORAGE_LOCKED();
    }
    this.numReads++;
    Assert.equal(this.numReads, 1, "should only ever be 1 read of unlocked data");
    return this.data;
  }),

  set: Task.async(function* (uid, contents) {
    this.data = contents;
  }),
}

function add_storage_task(testFunction) {
  add_task(function* () {
    print("Starting test with secure storage manager");
    yield testFunction(new FxAccountsStorageManager());
  });
  add_task(function* () {
    print("Starting test with simple storage manager");
    yield testFunction(new FxAccountsStorageManager({useSecure: false}));
  });
}


add_storage_task(function* checkInitializedEmpty(sm) {
  if (sm.secureStorage) {
    sm.secureStorage = new MockedSecureStorage(null);
  }
  yield sm.initialize();
  Assert.strictEqual((yield sm.getAccountData()), null);
  Assert.rejects(sm.updateAccountData({foo: "bar"}), "No user is logged in")
});



add_storage_task(function* checkNewUser(sm) {
  let initialAccountData = {
    uid: "uid",
    email: "someone@somewhere.com",
    kA: "kA",
  };
  sm.plainStorage = new MockedPlainStorage()
  if (sm.secureStorage) {
    sm.secureStorage = new MockedSecureStorage(null);
  }
  yield sm.initialize(initialAccountData);
  let accountData = yield sm.getAccountData();
  Assert.equal(accountData.uid, initialAccountData.uid);
  Assert.equal(accountData.email, initialAccountData.email);
  Assert.equal(accountData.kA, initialAccountData.kA);

  
  Assert.equal(sm.plainStorage.data.accountData.uid, initialAccountData.uid);
  Assert.equal(sm.plainStorage.data.accountData.email, initialAccountData.email);
  
  if (sm.secureStorage) {
    Assert.equal(sm.secureStorage.data.accountData.kA, initialAccountData.kA);
  } else {
    Assert.equal(sm.plainStorage.data.accountData.kA, initialAccountData.kA);
  }
});


add_storage_task(function* checkEverythingRead(sm) {
  sm.plainStorage = new MockedPlainStorage({uid: "uid", email: "someone@somewhere.com"})
  if (sm.secureStorage) {
    sm.secureStorage = new MockedSecureStorage(null);
  }
  yield sm.initialize();
  let accountData = yield sm.getAccountData();
  Assert.ok(accountData, "read account data");
  Assert.equal(accountData.uid, "uid");
  Assert.equal(accountData.email, "someone@somewhere.com");
  
  
  yield sm.updateAccountData({verified: true, foo: "bar", kA: "kA"});
  accountData = yield sm.getAccountData();
  Assert.equal(accountData.foo, "bar");
  Assert.equal(accountData.kA, "kA");
  
  yield sm._promiseStorageComplete; 
  
  Assert.equal(sm.plainStorage.data.accountData.verified, true);
  
  if (sm.secureStorage) {
    Assert.equal(sm.secureStorage.data.accountData.kA, "kA");
    Assert.equal(sm.secureStorage.data.accountData.foo, "bar");
  } else {
    Assert.equal(sm.plainStorage.data.accountData.kA, "kA");
    Assert.equal(sm.plainStorage.data.accountData.foo, "bar");
  }
});

add_storage_task(function* checkInvalidUpdates(sm) {
  sm.plainStorage = new MockedPlainStorage({uid: "uid", email: "someone@somewhere.com"})
  if (sm.secureStorage) {
    sm.secureStorage = new MockedSecureStorage(null);
  }
  Assert.rejects(sm.updateAccountData({uid: "another"}), "Can't change");
  Assert.rejects(sm.updateAccountData({email: "someoneelse"}), "Can't change");
});

add_storage_task(function* checkNullUpdatesRemovedUnlocked(sm) {
  if (sm.secureStorage) {
    sm.plainStorage = new MockedPlainStorage({uid: "uid", email: "someone@somewhere.com"})
    sm.secureStorage = new MockedSecureStorage({kA: "kA", kB: "kB"});
  } else {
    sm.plainStorage = new MockedPlainStorage({uid: "uid", email: "someone@somewhere.com",
                                              kA: "kA", kB: "kB"});
  }
  yield sm.initialize();

  yield sm.updateAccountData({kA: null});
  let accountData = yield sm.getAccountData();
  Assert.ok(!accountData.kA);
  Assert.equal(accountData.kB, "kB");
});

add_storage_task(function* checkDelete(sm) {
  if (sm.secureStorage) {
    sm.plainStorage = new MockedPlainStorage({uid: "uid", email: "someone@somewhere.com"})
    sm.secureStorage = new MockedSecureStorage({kA: "kA", kB: "kB"});
  } else {
    sm.plainStorage = new MockedPlainStorage({uid: "uid", email: "someone@somewhere.com",
                                              kA: "kA", kB: "kB"});
  }
  yield sm.initialize();

  yield sm.deleteAccountData();
  
  Assert.equal(sm.plainStorage.data, null);
  if (sm.secureStorage) {
    Assert.equal(sm.secureStorage.data, null);
  }
  
  Assert.equal((yield sm.getAccountData()), null);
});


add_task(function* checkNullUpdatesRemovedLocked() {
  let sm = new FxAccountsStorageManager();
  sm.plainStorage = new MockedPlainStorage({uid: "uid", email: "someone@somewhere.com"})
  sm.secureStorage = new MockedSecureStorage({kA: "kA", kB: "kB"});
  sm.secureStorage.locked = true;
  yield sm.initialize();

  yield sm.updateAccountData({kA: null});
  let accountData = yield sm.getAccountData();
  Assert.ok(!accountData.kA);
  
  Assert.ok(!accountData.kB);

  
  sm.secureStorage.locked = false;
  accountData = yield sm.getAccountData();
  Assert.ok(!accountData.kA);
  Assert.equal(accountData.kB, "kB");
  
  
  Assert.strictEqual(sm.secureStorage.data.accountData.kA, undefined);
  Assert.strictEqual(sm.secureStorage.data.accountData.kB, "kB");
});

add_task(function* checkEverythingReadSecure() {
  let sm = new FxAccountsStorageManager();
  sm.plainStorage = new MockedPlainStorage({uid: "uid", email: "someone@somewhere.com"})
  sm.secureStorage = new MockedSecureStorage({kA: "kA"});
  yield sm.initialize();

  let accountData = yield sm.getAccountData();
  Assert.ok(accountData, "read account data");
  Assert.equal(accountData.uid, "uid");
  Assert.equal(accountData.email, "someone@somewhere.com");
  Assert.equal(accountData.kA, "kA");
});

add_task(function* checkLockedUpdates() {
  let sm = new FxAccountsStorageManager();
  sm.plainStorage = new MockedPlainStorage({uid: "uid", email: "someone@somewhere.com"})
  sm.secureStorage = new MockedSecureStorage({kA: "old-kA", kB: "kB"});
  sm.secureStorage.locked = true;
  yield sm.initialize();

  let accountData = yield sm.getAccountData();
  
  Assert.ok(!accountData.kA);
  Assert.ok(!accountData.kB);
  
  sm.updateAccountData({kA: "new-kA"});
  accountData = yield sm.getAccountData();
  Assert.equal(accountData.kA, "new-kA");
  
  sm.secureStorage.locked = false;
  accountData = yield sm.getAccountData();
  
  Assert.equal(accountData.kA, "new-kA");
  Assert.equal(accountData.kB, "kB");
  
  Assert.strictEqual(sm.secureStorage.data.accountData.kA, "new-kA");
  Assert.strictEqual(sm.secureStorage.data.accountData.kB, "kB");
});






let setupStorageManagerForQueueTest = Task.async(function* () {
  let sm = new FxAccountsStorageManager();
  sm.plainStorage = new MockedPlainStorage({uid: "uid", email: "someone@somewhere.com"})
  sm.secureStorage = new MockedSecureStorage({kA: "kA"});
  sm.secureStorage.locked = true;
  yield sm.initialize();

  let resolveBlocked, rejectBlocked;
  let blockedPromise = new Promise((resolve, reject) => {
    resolveBlocked = resolve;
    rejectBlocked = reject;
  });

  sm._queueStorageOperation(() => blockedPromise);
  return {sm, blockedPromise, resolveBlocked, rejectBlocked}
});


add_task(function* checkQueueSemantics() {
  let { sm, resolveBlocked } = yield setupStorageManagerForQueueTest();

  
  let resolveSubsequent;
  let subsequentPromise = new Promise(resolve => {
    resolveSubsequent = resolve;
  });
  let subsequentCalled = false;

  sm._queueStorageOperation(() => {
    subsequentCalled = true;
    resolveSubsequent();
    return subsequentPromise;
  });

  
  Assert.ok(!subsequentCalled);

  
  resolveBlocked();

  
  yield subsequentPromise;
  Assert.ok(subsequentCalled);
  yield sm.finalize();
});


add_task(function* checkQueueSemanticsOnError() {
  let { sm, blockedPromise, rejectBlocked } = yield setupStorageManagerForQueueTest();

  let resolveSubsequent;
  let subsequentPromise = new Promise(resolve => {
    resolveSubsequent = resolve;
  });
  let subsequentCalled = false;

  sm._queueStorageOperation(() => {
    subsequentCalled = true;
    resolveSubsequent();
    return subsequentPromise;
  });

  
  Assert.ok(!subsequentCalled);

  
  
  rejectBlocked("oh no");

  
  yield subsequentPromise;
  Assert.ok(subsequentCalled);

  
  try {
    yield blockedPromise;
    Assert.ok(false, "expected this promise to reject");
  } catch (ex) {
    Assert.equal(ex, "oh no");
  }
  yield sm.finalize();
});



add_task(function* checkQueuedReadAndUpdate() {
  let { sm, resolveBlocked } = yield setupStorageManagerForQueueTest();
  
  
  let _doReadCalled = false;
  sm._doReadAndUpdateSecure = () => {
    _doReadCalled = true;
    return Promise.resolve();
  }

  let resultPromise = sm._maybeReadAndUpdateSecure();
  Assert.ok(!_doReadCalled);

  resolveBlocked();
  yield resultPromise;
  Assert.ok(_doReadCalled);
  yield sm.finalize();
});

add_task(function* checkQueuedWrite() {
  let { sm, resolveBlocked } = yield setupStorageManagerForQueueTest();
  
  let __writeCalled = false;
  sm.__write = () => {
    __writeCalled = true;
    return Promise.resolve();
  }

  let writePromise = sm._write();
  Assert.ok(!__writeCalled);

  resolveBlocked();
  yield writePromise;
  Assert.ok(__writeCalled);
  yield sm.finalize();
});

add_task(function* checkQueuedDelete() {
  let { sm, resolveBlocked } = yield setupStorageManagerForQueueTest();
  
  let _deleteCalled = false;
  sm._deleteAccountData = () => {
    _deleteCalled = true;
    return Promise.resolve();
  }

  let resultPromise = sm.deleteAccountData();
  Assert.ok(!_deleteCalled);

  resolveBlocked();
  yield resultPromise;
  Assert.ok(_deleteCalled);
  yield sm.finalize();
});

function run_test() {
  run_next_test();
}
