Cu.import("resource://services-sync/base_records/crypto.js");
Cu.import("resource://services-sync/base_records/keys.js");
Cu.import("resource://services-sync/base_records/wbo.js");
Cu.import("resource://services-sync/constants.js");
Cu.import("resource://services-sync/engines.js");
Cu.import("resource://services-sync/identity.js");
Cu.import("resource://services-sync/resource.js");
Cu.import("resource://services-sync/stores.js");
Cu.import("resource://services-sync/trackers.js");
Cu.import("resource://services-sync/util.js");







function SteamRecord(uri) {
  CryptoWrapper.call(this, uri);
}
SteamRecord.prototype = {
  __proto__: CryptoWrapper.prototype
};
Utils.deferGetSet(SteamRecord, "cleartext", ["denomination"]);

function SteamStore() {
  Store.call(this, "Steam");
  this.items = {};
}
SteamStore.prototype = {
  __proto__: Store.prototype,

  create: function Store_create(record) {
    this.items[record.id] = record.denomination;
  },

  remove: function Store_remove(record) {
    delete this.items[record.id];
  },

  update: function Store_update(record) {
    this.items[record.id] = record.denomination;
  },

  itemExists: function Store_itemExists(id) {
    return (id in this.items);
  },

  createRecord: function(id) {
    var record = new SteamRecord();
    record.id = id;
    record.denomination = this.items[id] || "Data for new record: " + id;
    return record;
  },

  changeItemID: function(oldID, newID) {
    this.items[newID] = this.items[oldID];
    delete this.items[oldID];
  },

  getAllIDs: function() {
    let ids = {};
    for (var id in this.items) {
      ids[id] = true;
    }
    return ids;
  },

  wipe: function() {
    this.items = {};
  }
};

function SteamTracker() {
  Tracker.call(this, "Steam");
}
SteamTracker.prototype = {
  __proto__: Tracker.prototype
};


function SteamEngine() {
  SyncEngine.call(this, "Steam");
}
SteamEngine.prototype = {
  __proto__: SyncEngine.prototype,
  _storeObj: SteamStore,
  _trackerObj: SteamTracker,
  _recordObj: SteamRecord,

  _findDupe: function(item) {
    for (let [id, value] in Iterator(this._store.items)) {
      if (item.denomination == value) {
        return id;
      }
    }
  }
};


function makeSteamEngine() {
  return new SteamEngine();
}

var syncTesting = new SyncTestingInfrastructure(makeSteamEngine);






function sync_httpd_setup(handlers) {
  handlers["/1.0/foo/storage/meta/global"]
      = (new ServerWBO('global', {})).handler();
  handlers["/1.0/foo/storage/keys/pubkey"]
      = (new ServerWBO('pubkey')).handler();
  handlers["/1.0/foo/storage/keys/privkey"]
      = (new ServerWBO('privkey')).handler();
  return httpd_setup(handlers);
}

function createAndUploadKeypair() {
  let storageURL = Svc.Prefs.get("clusterURL") + Svc.Prefs.get("storageAPI")
                   + "/" + ID.get("WeaveID").username + "/storage/";

  PubKeys.defaultKeyUri = storageURL + "keys/pubkey";
  PrivKeys.defaultKeyUri = storageURL + "keys/privkey";
  let keys = PubKeys.createKeypair(ID.get("WeaveCryptoID"),
                                   PubKeys.defaultKeyUri,
                                   PrivKeys.defaultKeyUri);
  PubKeys.uploadKeypair(keys);
}

function createAndUploadSymKey(url) {
  let symkey = Svc.Crypto.generateRandomKey();
  let pubkey = PubKeys.getDefaultKey();
  let meta = new CryptoMeta(url);
  meta.addUnwrappedKey(pubkey, symkey);
  let res = new Resource(meta.uri);
  res.put(meta);
}


function encryptPayload(cleartext) {
  if (typeof cleartext == "object") {
    cleartext = JSON.stringify(cleartext);
  }

  return {encryption: "http://localhost:8080/1.0/foo/storage/crypto/steam",
          ciphertext: cleartext, 
          IV: "irrelevant",
          hmac: Utils.sha256HMAC(cleartext, null)};
}
















function test_syncStartup_emptyOrOutdatedGlobalsResetsSync() {
  _("SyncEngine._syncStartup resets sync and wipes server data if there's no or an oudated global record");

  Svc.Prefs.set("clusterURL", "http://localhost:8080/");
  Svc.Prefs.set("username", "foo");
  let crypto_steam = new ServerWBO('steam');

  
  let collection = new ServerCollection();
  collection.wbos.flying = new ServerWBO(
      'flying', encryptPayload({id: 'flying',
                                denomination: "LNER Class A3 4472"}));
  collection.wbos.scotsman = new ServerWBO(
      'scotsman', encryptPayload({id: 'scotsman',
                                  denomination: "Flying Scotsman"}));

  let server = sync_httpd_setup({
      "/1.0/foo/storage/crypto/steam": crypto_steam.handler(),
      "/1.0/foo/storage/steam": collection.handler()
  });
  createAndUploadKeypair();

  let engine = makeSteamEngine();
  engine._store.items = {rekolok: "Rekonstruktionslokomotive"};
  try {

    
    do_check_eq(crypto_steam.payload, undefined);
    do_check_eq(engine._tracker.changedIDs["rekolok"], undefined);
    let metaGlobal = Records.get(engine.metaURL);
    do_check_eq(metaGlobal.payload.engines, undefined);
    do_check_true(!!collection.wbos.flying.payload);
    do_check_true(!!collection.wbos.scotsman.payload);

    engine.lastSync = Date.now() / 1000;
    engine._syncStartup();

    
    let engineData = metaGlobal.payload.engines["steam"];
    do_check_eq(engineData.version, engine.version);
    do_check_eq(engineData.syncID, engine.syncID);

    
    do_check_eq(engine.lastSync, 0);
    do_check_eq(collection.wbos.flying.payload, undefined);
    do_check_eq(collection.wbos.scotsman.payload, undefined);

    
    do_check_true(!!crypto_steam.payload);
    do_check_true(!!crypto_steam.data.keyring);

    
    do_check_eq(engine._tracker.changedIDs["rekolok"], 0);

  } finally {
    server.stop(function() {});
    Svc.Prefs.resetBranch("");
    Records.clearCache();
    CryptoMetas.clearCache();
    syncTesting = new SyncTestingInfrastructure(makeSteamEngine);
  }
}

function test_syncStartup_metaGet404() {
  _("SyncEngine._syncStartup resets sync and wipes server data if the symmetric key is missing 404");

  Svc.Prefs.set("clusterURL", "http://localhost:8080/");
  Svc.Prefs.set("username", "foo");

  
  let crypto_steam = new ServerWBO("steam");

  
  let engine = makeSteamEngine();
  let global = new ServerWBO("global",
                             {engines: {steam: {version: engine.version,
                                                syncID: engine.syncID}}});

  
  let collection = new ServerCollection();
  collection.wbos.flying = new ServerWBO(
      "flying", encryptPayload({id: "flying",
                                denomination: "LNER Class A3 4472"}));
  collection.wbos.scotsman = new ServerWBO(
      "scotsman", encryptPayload({id: "scotsman",
                                  denomination: "Flying Scotsman"}));

  let server = sync_httpd_setup({
      "/1.0/foo/storage/crypto/steam": crypto_steam.handler(),
      "/1.0/foo/storage/steam": collection.handler()
  });
  createAndUploadKeypair();

  try {

    _("Confirm initial environment");
    do_check_false(!!crypto_steam.payload);
    do_check_true(!!collection.wbos.flying.payload);
    do_check_true(!!collection.wbos.scotsman.payload);

    engine.lastSync = Date.now() / 1000;
    engine._syncStartup();

    _("Sync was reset and server data was wiped");
    do_check_eq(engine.lastSync, 0);
    do_check_eq(collection.wbos.flying.payload, undefined);
    do_check_eq(collection.wbos.scotsman.payload, undefined);

    _("New bulk key was uploaded");
    let key = crypto_steam.data.keyring["http://localhost:8080/1.0/foo/storage/keys/pubkey"];
    do_check_eq(key.wrapped, "fake-symmetric-key-0");
    do_check_eq(key.hmac, "fake-symmetric-key-0                                            ");

  } finally {
    server.stop(function() {});
    Svc.Prefs.resetBranch("");
    Records.clearCache();
    CryptoMetas.clearCache();
    syncTesting = new SyncTestingInfrastructure(makeSteamEngine);
  }
}

function test_syncStartup_failedMetaGet() {
  _("SyncEngine._syncStartup non-404 failures for getting cryptometa should stop sync");

  Svc.Prefs.set("clusterURL", "http://localhost:8080/");
  Svc.Prefs.set("username", "foo");
  let server = httpd_setup({
    "/1.0/foo/storage/crypto/steam": function(request, response) {
      response.setStatusLine(request.httpVersion, 405, "Method Not Allowed");
      response.bodyOutputStream.write("Fail!", 5);
    }
  });

  let engine = makeSteamEngine();
  try {

    _("Getting the cryptometa will fail and should set the appropriate failure");
    let error;
    try {
      engine._syncStartup();
    } catch (ex) {
      error = ex;
    }
    do_check_eq(error.failureCode, ENGINE_METARECORD_DOWNLOAD_FAIL);

  } finally {
    server.stop(function() {});
    Svc.Prefs.resetBranch("");
    Records.clearCache();
    syncTesting = new SyncTestingInfrastructure(makeSteamEngine);
  }
}

function test_syncStartup_serverHasNewerVersion() {
  _("SyncEngine._syncStartup ");

  Svc.Prefs.set("clusterURL", "http://localhost:8080/");
  Svc.Prefs.set("username", "foo");
  let global = new ServerWBO('global', {engines: {steam: {version: 23456}}});
  let server = httpd_setup({
      "/1.0/foo/storage/meta/global": global.handler()
  });

  let engine = makeSteamEngine();
  try {

    
    
    let error;
    try {
      engine._syncStartup();
    } catch (ex) {
      error = ex;
    }
    do_check_eq(error.failureCode, VERSION_OUT_OF_DATE);

  } finally {
    server.stop(function() {});
    Svc.Prefs.resetBranch("");
    Records.clearCache();
    syncTesting = new SyncTestingInfrastructure(makeSteamEngine);
  }
}


function test_syncStartup_syncIDMismatchResetsClient() {
  _("SyncEngine._syncStartup resets sync if syncIDs don't match");

  Svc.Prefs.set("clusterURL", "http://localhost:8080/");
  Svc.Prefs.set("username", "foo");
  let crypto_steam = new ServerWBO('steam');
  let server = sync_httpd_setup({
      "/1.0/foo/storage/crypto/steam": crypto_steam.handler()
  });

  
  let engine = makeSteamEngine();
  let global = new ServerWBO('global',
                             {engines: {steam: {version: engine.version,
                                                syncID: 'foobar'}}});
  server.registerPathHandler("/1.0/foo/storage/meta/global", global.handler());

  createAndUploadKeypair();

  try {

    
    do_check_eq(engine.syncID, 'fake-guid-0');
    do_check_eq(crypto_steam.payload, undefined);
    do_check_eq(engine._tracker.changedIDs["rekolok"], undefined);

    engine.lastSync = Date.now() / 1000;
    engine._syncStartup();

    
    do_check_eq(engine.syncID, 'foobar');

    
    do_check_eq(engine.lastSync, 0);

  } finally {
    server.stop(function() {});
    Svc.Prefs.resetBranch("");
    Records.clearCache();
    CryptoMetas.clearCache();
    syncTesting = new SyncTestingInfrastructure(makeSteamEngine);
  }
}


function test_syncStartup_badKeyWipesServerData() {
  _("SyncEngine._syncStartup resets sync and wipes server data if there's something wrong with the symmetric key");

  Svc.Prefs.set("clusterURL", "http://localhost:8080/");
  Svc.Prefs.set("username", "foo");

  
  let crypto_steam = new ServerWBO('steam');
  crypto_steam.payload = JSON.stringify({
    keyring: {
      "http://localhost:8080/1.0/foo/storage/keys/pubkey": {
        wrapped: Svc.Crypto.generateRandomKey(),
        hmac: "this-hmac-is-incorrect"
      }
    }
  });

  
  let engine = makeSteamEngine();
  let global = new ServerWBO('global',
                             {engines: {steam: {version: engine.version,
                                                syncID: engine.syncID}}});

  
  let collection = new ServerCollection();
  collection.wbos.flying = new ServerWBO(
      'flying', encryptPayload({id: 'flying',
                                denomination: "LNER Class A3 4472"}));
  collection.wbos.scotsman = new ServerWBO(
      'scotsman', encryptPayload({id: 'scotsman',
                                  denomination: "Flying Scotsman"}));

  let server = sync_httpd_setup({
      "/1.0/foo/storage/crypto/steam": crypto_steam.handler(),
      "/1.0/foo/storage/steam": collection.handler()
  });
  createAndUploadKeypair();

  try {

    
    let key = crypto_steam.data.keyring["http://localhost:8080/1.0/foo/storage/keys/pubkey"];
    do_check_eq(key.wrapped, "fake-symmetric-key-0");
    do_check_eq(key.hmac, "this-hmac-is-incorrect");
    do_check_true(!!collection.wbos.flying.payload);
    do_check_true(!!collection.wbos.scotsman.payload);

    engine.lastSync = Date.now() / 1000;
    engine._syncStartup();

    
    do_check_eq(engine.lastSync, 0);
    do_check_eq(collection.wbos.flying.payload, undefined);
    do_check_eq(collection.wbos.scotsman.payload, undefined);

    
    key = crypto_steam.data.keyring["http://localhost:8080/1.0/foo/storage/keys/pubkey"];
    do_check_eq(key.wrapped, "fake-symmetric-key-1");
    do_check_eq(key.hmac, "fake-symmetric-key-1                                            ");

  } finally {
    server.stop(function() {});
    Svc.Prefs.resetBranch("");
    Records.clearCache();
    CryptoMetas.clearCache();
    syncTesting = new SyncTestingInfrastructure(makeSteamEngine);
  }
}


function test_processIncoming_emptyServer() {
  _("SyncEngine._processIncoming working with an empty server backend");

  Svc.Prefs.set("clusterURL", "http://localhost:8080/");
  Svc.Prefs.set("username", "foo");
  let crypto_steam = new ServerWBO('steam');
  let collection = new ServerCollection();

  let server = sync_httpd_setup({
      "/1.0/foo/storage/crypto/steam": crypto_steam.handler(),
      "/1.0/foo/storage/steam": collection.handler()
  });
  createAndUploadKeypair();

  let engine = makeSteamEngine();
  try {

    
    engine._processIncoming();
    do_check_eq(engine.lastSync, 0);
    do_check_eq(engine.toFetch.length, 0);

  } finally {
    server.stop(function() {});
    Svc.Prefs.resetBranch("");
    Records.clearCache();
    CryptoMetas.clearCache();
    syncTesting = new SyncTestingInfrastructure(makeSteamEngine);
  }
}


function test_processIncoming_createFromServer() {
  _("SyncEngine._processIncoming creates new records from server data");

  Svc.Prefs.set("clusterURL", "http://localhost:8080/");
  Svc.Prefs.set("username", "foo");
  let crypto_steam = new ServerWBO('steam');

  
  let collection = new ServerCollection();
  collection.wbos.flying = new ServerWBO(
      'flying', encryptPayload({id: 'flying',
                                denomination: "LNER Class A3 4472"}));
  collection.wbos.scotsman = new ServerWBO(
      'scotsman', encryptPayload({id: 'scotsman',
                                  denomination: "Flying Scotsman"}));

  let server = sync_httpd_setup({
      "/1.0/foo/storage/crypto/steam": crypto_steam.handler(),
      "/1.0/foo/storage/steam": collection.handler(),
      "/1.0/foo/storage/steam/flying": collection.wbos.flying.handler(),
      "/1.0/foo/storage/steam/scotsman": collection.wbos.scotsman.handler()
  });
  createAndUploadKeypair();
  createAndUploadSymKey("http://localhost:8080/1.0/foo/storage/crypto/steam");

  let engine = makeSteamEngine();
  try {

    
    do_check_eq(engine.lastSync, 0);
    do_check_eq(engine.lastModified, null);
    do_check_eq(engine._store.items.flying, undefined);
    do_check_eq(engine._store.items.scotsman, undefined);

    engine._processIncoming();

    
    do_check_true(engine.lastSync > 0);
    do_check_true(engine.lastModified > 0);

    
    do_check_eq(engine._store.items.flying, "LNER Class A3 4472");
    do_check_eq(engine._store.items.scotsman, "Flying Scotsman");

  } finally {
    server.stop(function() {});
    Svc.Prefs.resetBranch("");
    Records.clearCache();
    CryptoMetas.clearCache();
    syncTesting = new SyncTestingInfrastructure(makeSteamEngine);
  }
}


function test_processIncoming_reconcile() {
  _("SyncEngine._processIncoming updates local records");

  Svc.Prefs.set("clusterURL", "http://localhost:8080/");
  Svc.Prefs.set("username", "foo");
  let crypto_steam = new ServerWBO('steam');
  let collection = new ServerCollection();

  
  
  collection.wbos.newrecord = new ServerWBO(
      'newrecord', encryptPayload({id: 'newrecord',
                                   denomination: "New stuff..."}));

  
  
  collection.wbos.newerserver = new ServerWBO(
      'newerserver', encryptPayload({id: 'newerserver',
                                     denomination: "New data!"}));

  
  
  
  collection.wbos.olderidentical = new ServerWBO(
      'olderidentical', encryptPayload({id: 'olderidentical',
                                        denomination: "Older but identical"}));
  collection.wbos.olderidentical.modified -= 120;

  
  
  collection.wbos.updateclient = new ServerWBO(
      'updateclient', encryptPayload({id: 'updateclient',
                                      denomination: "Get this!"}));

  
  
  collection.wbos.duplication = new ServerWBO(
      'duplication', encryptPayload({id: 'duplication',
                                     denomination: "Original Entry"}));

  
  
  collection.wbos.dupe = new ServerWBO(
      'dupe', encryptPayload({id: 'dupe',
                              denomination: "Long Original Entry"}));  

  
  
  collection.wbos.nukeme = new ServerWBO(
      'nukeme', encryptPayload({id: 'nukeme',
                                denomination: "Nuke me!",
                                deleted: true}));

  let server = sync_httpd_setup({
      "/1.0/foo/storage/crypto/steam": crypto_steam.handler(),
      "/1.0/foo/storage/steam": collection.handler()
  });
  createAndUploadKeypair();
  createAndUploadSymKey("http://localhost:8080/1.0/foo/storage/crypto/steam");

  let engine = makeSteamEngine();
  engine._store.items = {newerserver: "New data, but not as new as server!",
                         olderidentical: "Older but identical",
                         updateclient: "Got data?",
                         original: "Original Entry",
                         long_original: "Long Original Entry",
                         nukeme: "Nuke me!"};
  
  engine._tracker.addChangedID('newerserver', Date.now()/1000 - 60);
  
  engine._tracker.addChangedID('olderidentical', Date.now()/1000);

  try {

    
    do_check_eq(engine._store.items.newrecord, undefined);
    do_check_eq(engine._store.items.newerserver, "New data, but not as new as server!");
    do_check_eq(engine._store.items.olderidentical, "Older but identical");
    do_check_eq(engine._store.items.updateclient, "Got data?");
    do_check_eq(engine._store.items.nukeme, "Nuke me!");
    do_check_true(engine._tracker.changedIDs['olderidentical'] > 0);

    engine._delete = {}; 
    engine._processIncoming();

    
    do_check_true(engine.lastSync > 0);
    do_check_true(engine.lastModified > 0);

    
    do_check_eq(engine._store.items.newrecord, "New stuff...");

    
    do_check_eq(engine._store.items.newerserver, "New data!");

    
    
    do_check_eq(engine._store.items.olderidentical, "Older but identical");
    do_check_eq(engine._tracker.changedIDs['olderidentical'], undefined);

    
    do_check_eq(engine._store.items.updateclient, "Get this!");

    
    
    do_check_eq(engine._store.items.long_original, undefined);
    do_check_eq(engine._store.items.dupe, "Long Original Entry");
    do_check_neq(engine._delete.ids.indexOf('duplication'), -1);

    
    do_check_eq(engine._store.items.nukeme, undefined);

  } finally {
    server.stop(function() {});
    Svc.Prefs.resetBranch("");
    Records.clearCache();
    CryptoMetas.clearCache();
    syncTesting = new SyncTestingInfrastructure(makeSteamEngine);
  }
}


function test_processIncoming_fetchNum() {
  _("SyncEngine._processIncoming doesn't fetch everything at ones on mobile clients");

  Svc.Prefs.set("clusterURL", "http://localhost:8080/");
  Svc.Prefs.set("username", "foo");
  Svc.Prefs.set("client.type", "mobile");
  let crypto_steam = new ServerWBO('steam');
  let collection = new ServerCollection();

  
  
  for (var i = 0; i < 234; i++) {
    let id = 'record-no-' + i;
    let payload = encryptPayload({id: id, denomination: "Record No. " + i});
    let wbo = new ServerWBO(id, payload);
    wbo.modified = Date.now()/1000 - 60*(i+10);
    collection.wbos[id] = wbo;
  }

  let server = sync_httpd_setup({
      "/1.0/foo/storage/crypto/steam": crypto_steam.handler(),
      "/1.0/foo/storage/steam": collection.handler()
  });
  createAndUploadKeypair();
  createAndUploadSymKey("http://localhost:8080/1.0/foo/storage/crypto/steam");

  let engine = makeSteamEngine();

  try {

    
    
    engine._processIncoming();
    do_check_eq([id for (id in engine._store.items)].length, 50);
    do_check_true('record-no-0' in engine._store.items);
    do_check_true('record-no-49' in engine._store.items);
    do_check_eq(engine.toFetch.length, 234 - 50);


    
    
    engine._processIncoming();
    do_check_eq([id for (id in engine._store.items)].length, 100);
    do_check_true('record-no-50' in engine._store.items);
    do_check_true('record-no-99' in engine._store.items);
    do_check_eq(engine.toFetch.length, 234 - 100);


    
    for (i=0; i < 5; i++) {
      let id = 'new-record-no-' + i;
      let payload = encryptPayload({id: id, denomination: "New record No. " + i});
      let wbo = new ServerWBO(id, payload);
      wbo.modified = Date.now()/1000 - 60*i;
      collection.wbos[id] = wbo;
    }
    
    
    engine.lastModified = Date.now() / 1000 + 1;

    
    
    engine._processIncoming();
    do_check_eq([id for (id in engine._store.items)].length, 150);
    do_check_true('new-record-no-0' in engine._store.items);
    do_check_true('new-record-no-4' in engine._store.items);
    do_check_true('record-no-100' in engine._store.items);
    do_check_true('record-no-144' in engine._store.items);
    do_check_eq(engine.toFetch.length, 234 - 100 - 45);


    
    
    collection.wbos['record-no-3'].modified = Date.now()/1000 + 1;
    collection.wbos['record-no-41'].modified = Date.now()/1000 + 1;
    collection.wbos['record-no-122'].modified = Date.now()/1000 + 1;

    
    
    
    engine.lastModified = Date.now() / 1000 + 2;
    engine._processIncoming();
    do_check_eq([id for (id in engine._store.items)].length, 197);
    do_check_true('record-no-145' in engine._store.items);
    do_check_true('record-no-191' in engine._store.items);
    do_check_eq(engine.toFetch.length, 234 - 100 - 45 - 47);


    
    
    while(engine.toFetch.length) {
      engine._processIncoming();
    }
    do_check_eq([id for (id in engine._store.items)].length, 234 + 5);
    do_check_true('record-no-233' in engine._store.items);

  } finally {
    server.stop(function() {});
    Svc.Prefs.resetBranch("");
    Records.clearCache();
    CryptoMetas.clearCache();
    syncTesting = new SyncTestingInfrastructure(makeSteamEngine);
  }
}


function test_uploadOutgoing_toEmptyServer() {
  _("SyncEngine._uploadOutgoing uploads new records to server");

  Svc.Prefs.set("clusterURL", "http://localhost:8080/");
  Svc.Prefs.set("username", "foo");
  let crypto_steam = new ServerWBO('steam');
  let collection = new ServerCollection();
  collection.wbos.flying = new ServerWBO('flying');
  collection.wbos.scotsman = new ServerWBO('scotsman');

  let server = sync_httpd_setup({
      "/1.0/foo/storage/crypto/steam": crypto_steam.handler(),
      "/1.0/foo/storage/steam": collection.handler(),
      "/1.0/foo/storage/steam/flying": collection.wbos.flying.handler(),
      "/1.0/foo/storage/steam/scotsman": collection.wbos.scotsman.handler()
  });
  createAndUploadKeypair();
  createAndUploadSymKey("http://localhost:8080/1.0/foo/storage/crypto/steam");

  let engine = makeSteamEngine();
  engine._store.items = {flying: "LNER Class A3 4472",
                         scotsman: "Flying Scotsman"};
  
  engine._tracker.addChangedID('scotsman', 0);

  try {

    
    do_check_eq(collection.wbos.flying.payload, undefined);
    do_check_eq(collection.wbos.scotsman.payload, undefined);
    do_check_eq(engine._tracker.changedIDs['scotsman'], 0);

    engine._uploadOutgoing();

    
    
    do_check_eq(collection.wbos.flying.payload, undefined);
    do_check_true(!!collection.wbos.scotsman.payload);
    do_check_eq(JSON.parse(collection.wbos.scotsman.data.ciphertext).id,
                'scotsman');
    do_check_eq(engine._tracker.changedIDs['scotsman'], undefined);

    
    do_check_eq(collection.wbos.flying.payload, undefined);

  } finally {
    server.stop(function() {});
    Svc.Prefs.resetBranch("");
    Records.clearCache();
    CryptoMetas.clearCache();
    syncTesting = new SyncTestingInfrastructure(makeSteamEngine);
  }
}


function test_uploadOutgoing_MAX_UPLOAD_RECORDS() {
  _("SyncEngine._uploadOutgoing uploads in batches of MAX_UPLOAD_RECORDS");

  Svc.Prefs.set("clusterURL", "http://localhost:8080/");
  Svc.Prefs.set("username", "foo");
  let crypto_steam = new ServerWBO('steam');
  let collection = new ServerCollection();

  
  var noOfUploads = 0;
  collection.post = (function(orig) {
    return function() {
      noOfUploads++;
      return orig.apply(this, arguments);
    };
  }(collection.post));

  
  let engine = makeSteamEngine();
  for (var i = 0; i < 234; i++) {
    let id = 'record-no-' + i;
    engine._store.items[id] = "Record No. " + i;
    engine._tracker.addChangedID(id, 0);
    collection.wbos[id] = new ServerWBO(id);
  }

  let server = sync_httpd_setup({
      "/1.0/foo/storage/crypto/steam": crypto_steam.handler(),
      "/1.0/foo/storage/steam": collection.handler()
  });
  createAndUploadKeypair();
  createAndUploadSymKey("http://localhost:8080/1.0/foo/storage/crypto/steam");

  try {

    
    do_check_eq(noOfUploads, 0);

    engine._uploadOutgoing();

    
    for (i = 0; i < 234; i++) {
      do_check_true(!!collection.wbos['record-no-'+i].payload);
    }

    
    do_check_eq(noOfUploads, Math.ceil(234/MAX_UPLOAD_RECORDS));

  } finally {
    server.stop(function() {});
    Svc.Prefs.resetBranch("");
    Records.clearCache();
    CryptoMetas.clearCache();
    syncTesting = new SyncTestingInfrastructure(makeSteamEngine);
  }
}


function test_syncFinish_noDelete() {
  _("SyncEngine._syncFinish resets tracker's score");
  let engine = makeSteamEngine();
  engine._delete = {}; 
  engine._tracker.score = 100;

  
  engine._syncFinish();
  do_check_eq(engine.score, 0);
}


function test_syncFinish_deleteByIds() {
  _("SyncEngine._syncFinish deletes server records slated for deletion (list of record IDs).");

  Svc.Prefs.set("clusterURL", "http://localhost:8080/");
  Svc.Prefs.set("username", "foo");
  let collection = new ServerCollection();
  collection.wbos.flying = new ServerWBO(
      'flying', encryptPayload({id: 'flying',
                                denomination: "LNER Class A3 4472"}));
  collection.wbos.scotsman = new ServerWBO(
      'scotsman', encryptPayload({id: 'scotsman',
                                  denomination: "Flying Scotsman"}));
  collection.wbos.rekolok = new ServerWBO(
      'rekolok', encryptPayload({id: 'rekolok',
                                denomination: "Rekonstruktionslokomotive"}));

  let server = httpd_setup({
      "/1.0/foo/storage/steam": collection.handler()
  });

  let engine = makeSteamEngine();
  try {
    engine._delete = {ids: ['flying', 'rekolok']};
    engine._syncFinish();

    
    
    do_check_eq(collection.wbos.flying.payload, undefined);
    do_check_true(!!collection.wbos.scotsman.payload);
    do_check_eq(collection.wbos.rekolok.payload, undefined);

    
    do_check_eq(engine._delete.ids, undefined);

  } finally {
    server.stop(function() {});
    Svc.Prefs.resetBranch("");
    Records.clearCache();
    syncTesting = new SyncTestingInfrastructure(makeSteamEngine);
  }
}


function test_syncFinish_deleteLotsInBatches() {
  _("SyncEngine._syncFinish deletes server records in batches of 100 (list of record IDs).");

  Svc.Prefs.set("clusterURL", "http://localhost:8080/");
  Svc.Prefs.set("username", "foo");
  let collection = new ServerCollection();

  
  var noOfUploads = 0;
  collection.delete = (function(orig) {
    return function() {
      noOfUploads++;
      return orig.apply(this, arguments);
    };
  }(collection.delete));

  
  let now = Date.now();
  for (var i = 0; i < 234; i++) {
    let id = 'record-no-' + i;
    let payload = encryptPayload({id: id, denomination: "Record No. " + i});
    let wbo = new ServerWBO(id, payload);
    wbo.modified = now / 1000 - 60 * (i + 110);
    collection.wbos[id] = wbo;
  }

  let server = httpd_setup({
      "/1.0/foo/storage/steam": collection.handler()
  });

  let engine = makeSteamEngine();
  try {

    
    do_check_eq(noOfUploads, 0);

    
    
    
    engine._delete = {ids: [],
                      newer: now / 1000 - 60 * 200.5};
    for (i = 100; i < 234; i++) {
      engine._delete.ids.push('record-no-' + i);
    }

    engine._syncFinish();

    
    
    for (i = 0; i < 234; i++) {
      let id = 'record-no-' + i;
      if (i <= 90 || i >= 100) {
        do_check_eq(collection.wbos[id].payload, undefined);
      } else {
        do_check_true(!!collection.wbos[id].payload);
      }
    }

    
    do_check_eq(noOfUploads, 2 + 1);

    
    do_check_eq(engine._delete.ids, undefined);

  } finally {
    server.stop(function() {});
    Svc.Prefs.resetBranch("");
    Records.clearCache();
    syncTesting = new SyncTestingInfrastructure(makeSteamEngine);
  }
}

function run_test() {
  test_syncStartup_emptyOrOutdatedGlobalsResetsSync();
  test_syncStartup_metaGet404();
  test_syncStartup_failedMetaGet();
  test_syncStartup_serverHasNewerVersion();
  test_syncStartup_syncIDMismatchResetsClient();
  test_syncStartup_badKeyWipesServerData();
  test_processIncoming_emptyServer();
  test_processIncoming_createFromServer();
  test_processIncoming_reconcile();
  test_processIncoming_fetchNum();
  test_uploadOutgoing_toEmptyServer();
  test_uploadOutgoing_MAX_UPLOAD_RECORDS();
  test_syncFinish_noDelete();
  test_syncFinish_deleteByIds();
  test_syncFinish_deleteLotsInBatches();
}
