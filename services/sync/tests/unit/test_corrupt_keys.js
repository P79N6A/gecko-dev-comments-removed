


Cu.import("resource://gre/modules/PlacesUtils.jsm");
Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/engines/tabs.js");
Cu.import("resource://services-sync/engines/history.js");
Cu.import("resource://services-sync/record.js");
Cu.import("resource://services-sync/service.js");
Cu.import("resource://services-sync/status.js");
Cu.import("resource://services-sync/util.js");
Cu.import("resource://testing-common/services/sync/utils.js");
Cu.import("resource://gre/modules/Promise.jsm");

add_task(function test_locally_changed_keys() {
  let passphrase = "abcdeabcdeabcdeabcdeabcdea";

  let hmacErrorCount = 0;
  function counting(f) {
    return function() {
      hmacErrorCount++;
      return f.call(this);
    };
  }

  Service.handleHMACEvent = counting(Service.handleHMACEvent);

  let server  = new SyncServer();
  let johndoe = server.registerUser("johndoe", "password");
  johndoe.createContents({
    meta: {},
    crypto: {},
    clients: {}
  });
  server.start();

  try {
    Svc.Prefs.set("registerEngines", "Tab");
    _("Set up some tabs.");
    let myTabs =
      {windows: [{tabs: [{index: 1,
                          entries: [{
                            url: "http://foo.com/",
                            title: "Title"
                          }],
                          attributes: {
                            image: "image"
                          }
                          }]}]};
    delete Svc.Session;
    Svc.Session = {
      getBrowserState: function () JSON.stringify(myTabs)
    };

    setBasicCredentials("johndoe", "password", passphrase);
    Service.serverURL = server.baseURI;
    Service.clusterURL = server.baseURI;

    Service.engineManager.register(HistoryEngine);

    function corrupt_local_keys() {
      Service.collectionKeys._default.keyPair = [Svc.Crypto.generateRandomKey(),
                                                 Svc.Crypto.generateRandomKey()];
    }

    _("Setting meta.");

    
    let m = new WBORecord("meta", "global");
    m.payload = {"syncID": "foooooooooooooooooooooooooo",
                 "storageVersion": STORAGE_VERSION};
    m.upload(Service.resource(Service.metaURL));

    _("New meta/global: " + JSON.stringify(johndoe.collection("meta").wbo("global")));

    
    generateNewKeys(Service.collectionKeys);
    let serverKeys = Service.collectionKeys.asWBO("crypto", "keys");
    serverKeys.encrypt(Service.identity.syncKeyBundle);
    do_check_true(serverKeys.upload(Service.resource(Service.cryptoKeysURL)).success);

    
    do_check_true(Service.login("johndoe", "ilovejane", passphrase));
    do_check_true(Service.isLoggedIn);

    
    Service.sync();

    
    _("Tabs modified: " + johndoe.modified("tabs"));
    do_check_true(johndoe.modified("tabs") > 0);

    let coll_modified = Service.collectionKeys.lastModified;

    
    let liveKeys = Service.collectionKeys.keyForCollection("history");
    _("Keys now: " + liveKeys.keyPair);
    let visitType = Ci.nsINavHistoryService.TRANSITION_LINK;
    let history   = johndoe.createCollection("history");
    for (let i = 0; i < 5; i++) {
      let id = 'record-no--' + i;
      let modified = Date.now()/1000 - 60*(i+10);

      let w = new CryptoWrapper("history", "id");
      w.cleartext = {
        id: id,
        histUri: "http://foo/bar?" + id,
        title: id,
        sortindex: i,
        visits: [{date: (modified - 5) * 1000000, type: visitType}],
        deleted: false};
      w.encrypt(liveKeys);

      let payload = {ciphertext: w.ciphertext,
                     IV:         w.IV,
                     hmac:       w.hmac};
      history.insert(id, payload, modified);
    }

    history.timestamp = Date.now() / 1000;
    let old_key_time = johndoe.modified("crypto");
    _("Old key time: " + old_key_time);

    
    let rec = new CryptoWrapper("history", "record-no--0");
    rec.fetch(Service.resource(Service.storageURL + "history/record-no--0"));
    _(JSON.stringify(rec));
    do_check_true(!!rec.decrypt(liveKeys));

    do_check_eq(hmacErrorCount, 0);

    
    corrupt_local_keys();
    _("Keys now: " + Service.collectionKeys.keyForCollection("history").keyPair);

    do_check_eq(hmacErrorCount, 0);

    _("HMAC error count: " + hmacErrorCount);
    
    Service.sync();
    do_check_eq(hmacErrorCount, 1);
    _("Keys now: " + Service.collectionKeys.keyForCollection("history").keyPair);

    
    let store = Service.engineManager.get("history")._store;
    do_check_true(yield promiseIsURIVisited("http://foo/bar?record-no--0"));
    do_check_true(yield promiseIsURIVisited("http://foo/bar?record-no--1"));
    do_check_true(yield promiseIsURIVisited("http://foo/bar?record-no--2"));
    do_check_true(yield promiseIsURIVisited("http://foo/bar?record-no--3"));
    do_check_true(yield promiseIsURIVisited("http://foo/bar?record-no--4"));
    do_check_eq(hmacErrorCount, 1);

    _("Busting some new server values.");
    
    for (let i = 5; i < 10; i++) {
      let id = 'record-no--' + i;
      let modified = 1 + (Date.now() / 1000);

      let w = new CryptoWrapper("history", "id");
      w.cleartext = {
        id: id,
        histUri: "http://foo/bar?" + id,
        title: id,
        sortindex: i,
        visits: [{date: (modified - 5 ) * 1000000, type: visitType}],
        deleted: false};
      w.encrypt(Service.collectionKeys.keyForCollection("history"));
      w.hmac = w.hmac.toUpperCase();

      let payload = {ciphertext: w.ciphertext,
                     IV:         w.IV,
                     hmac:       w.hmac};
      history.insert(id, payload, modified);
    }
    history.timestamp = Date.now() / 1000;

    _("Server key time hasn't changed.");
    do_check_eq(johndoe.modified("crypto"), old_key_time);

    _("Resetting HMAC error timer.");
    Service.lastHMACEvent = 0;

    _("Syncing...");
    Service.sync();
    _("Keys now: " + Service.collectionKeys.keyForCollection("history").keyPair);
    _("Server keys have been updated, and we skipped over 5 more HMAC errors without adjusting history.");
    do_check_true(johndoe.modified("crypto") > old_key_time);
    do_check_eq(hmacErrorCount, 6);
    do_check_false(yield promiseIsURIVisited("http://foo/bar?record-no--5"));
    do_check_false(yield promiseIsURIVisited("http://foo/bar?record-no--6"));
    do_check_false(yield promiseIsURIVisited("http://foo/bar?record-no--7"));
    do_check_false(yield promiseIsURIVisited("http://foo/bar?record-no--8"));
    do_check_false(yield promiseIsURIVisited("http://foo/bar?record-no--9"));
  } finally {
    Svc.Prefs.resetBranch("");
    let deferred = Promise.defer();
    server.stop(deferred.resolve);
    yield deferred.promise;
  }
});

function run_test() {
  let logger = Log.repository.rootLogger;
  Log.repository.rootLogger.addAppender(new Log.DumpAppender());

  ensureLegacyIdentityManager();

  run_next_test();
}








function promiseIsURIVisited(url) {
  let deferred = Promise.defer();
  PlacesUtils.asyncHistory.isURIVisited(Utils.makeURI(url), function(aURI, aIsVisited) {
    deferred.resolve(aIsVisited);
  });

  return deferred.promise;
}
