



Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/FxAccounts.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");

Cu.import("resource://testing-common/services/sync/utils.js");
Cu.import("resource://testing-common/services/common/logging.js");

Cu.import("resource://services-sync/record.js");


Services.prefs.setCharPref("services.sync.username", "foo");


Cu.import("resource://services-sync/service.js");

const USER = "foo";
const PASSPHRASE = "abcdeabcdeabcdeabcdeabcdea";

function promiseStopServer(server) {
  return new Promise((resolve, reject) => {
    server.stop(resolve);
  });
}

let numServerRequests = 0;


function configureLegacySync() {
  let contents = {
    meta: {global: {}},
    crypto: {},
  };

  setBasicCredentials(USER, "password", PASSPHRASE);

  numServerRequests = 0;
  let server = new SyncServer({
    onRequest: () => {
      ++numServerRequests
    }
  });
  server.registerUser(USER, "password");
  server.createContents(USER, contents);
  server.start();

  Service.serverURL = server.baseURI;
  Service.clusterURL = server.baseURI;
  Service.identity.username = USER;
  Service._updateCachedURLs();

  return server;
}


add_task(function *() {
  
  let server = configureLegacySync();

  Assert.equal((yield Service.getFxAMigrationSentinel()), null, "no sentinel to start");

  let sentinel = {foo: "bar"};
  yield Service.setFxAMigrationSentinel(sentinel);

  Assert.deepEqual((yield Service.getFxAMigrationSentinel()), sentinel, "got the sentinel back");

  yield promiseStopServer(server);
});


add_task(function *() {
  
  let server = configureLegacySync();
  Service.login();

  
  numServerRequests = 0;

  Assert.equal((yield Service.getFxAMigrationSentinel()), null, "no sentinel to start");
  Assert.equal(numServerRequests, 1, "first fetch should hit the server");

  let sentinel = {foo: "bar"};
  yield Service.setFxAMigrationSentinel(sentinel);
  Assert.equal(numServerRequests, 2, "setting sentinel should hit the server");

  Assert.deepEqual((yield Service.getFxAMigrationSentinel()), sentinel, "got the sentinel back");
  Assert.equal(numServerRequests, 2, "second fetch should not should hit the server");

  
  
  Service.recordManager.clearCache();
  Assert.deepEqual((yield Service.getFxAMigrationSentinel()), sentinel, "got the sentinel back");
  Assert.equal(numServerRequests, 3, "should have re-hit the server with empty caches");

  yield promiseStopServer(server);
});


add_task(function* () {
  let server = configureLegacySync();

  
  
  Service.sync();

  
  
  let cryptoWrapper = new CryptoWrapper("meta", "fxa_credentials");
  let sentinel = {foo: "bar"};
  cryptoWrapper.cleartext = {
    id: "fxa_credentials",
    sentinel: sentinel,
    deleted: false,
  }
  cryptoWrapper.encrypt(Service.identity.syncKeyBundle);
  let payload = {
    ciphertext: cryptoWrapper.ciphertext,
    IV:         cryptoWrapper.IV,
    hmac:       cryptoWrapper.hmac,
  };

  server.createContents(USER, {
    meta: {fxa_credentials: payload},
    crypto: {},
  });

  
  Service.sync();
  
  numServerRequests = 0;

  
  Assert.deepEqual((yield Service.getFxAMigrationSentinel()), sentinel, "got it");
  Assert.equal(numServerRequests, 0, "should not have hit the server");

  
  
  Assert.deepEqual((yield Service.getFxAMigrationSentinel()), sentinel, "got it again");
  Assert.equal(numServerRequests, 0, "should not have hit the server");

  yield promiseStopServer(server);
});

function run_test() {
  initTestLogging();
  run_next_test();
}
