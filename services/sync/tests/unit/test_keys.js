var btoa;

Cu.import("resource://services-sync/base_records/crypto.js");
Cu.import("resource://services-sync/constants.js");
btoa = Cu.import("resource://services-sync/util.js").btoa;

function test_time_keyFromString(iterations) {
  let k;
  let o;
  let b = new BulkKeyBundle();
  let d = Utils.decodeKeyBase32("ababcdefabcdefabcdefabcdef");
  b.generateRandom();
  
  _("Running " + iterations + " iterations of hmacKeyObject + sha256HMACBytes.");
  for (let i = 0; i < iterations; ++i) {
    let k = b.hmacKeyObject;
    o = Utils.sha256HMACBytes(d, k);
  }
  do_check_true(!!o);
  _("Done.");
}

function test_repeated_hmac() {
  let testKey = "ababcdefabcdefabcdefabcdef";
  let k = Utils.makeHMACKey("foo");
  let one = Utils.sha256HMACBytes(Utils.decodeKeyBase32(testKey), k);
  let two = Utils.sha256HMACBytes(Utils.decodeKeyBase32(testKey), k);
  do_check_eq(one, two);
}

function test_keymanager() {
  let testKey = "ababcdefabcdefabcdefabcdef";
  
  let username = "john@example.com";
  
  
  
  
  let sha256inputE = Utils.makeHMACKey("" + HMAC_INPUT + username + "\x01");
  let encryptKey = Utils.sha256HMACBytes(Utils.decodeKeyBase32(testKey), sha256inputE);
  
  let sha256inputH = Utils.makeHMACKey(encryptKey + HMAC_INPUT + username + "\x02");
  let hmacKey = Utils.sha256HMACBytes(Utils.decodeKeyBase32(testKey), sha256inputH);
  
  
  do_check_eq(btoa(encryptKey), new SyncKeyBundle(null, username, testKey).encryptionKey);
  do_check_eq(hmacKey,          new SyncKeyBundle(null, username, testKey).hmacKey);
  
  
  let obj = new SyncKeyBundle(null, username, testKey);
  do_check_eq(hmacKey, obj.hmacKey);
  do_check_eq(btoa(encryptKey), obj.encryptionKey);
}

function do_check_keypair_eq(a, b) {
  do_check_eq(2, a.length);
  do_check_eq(2, b.length);
  do_check_eq(a[0], b[0]);
  do_check_eq(a[1], b[1]);
}

function test_collections_manager() {
  let log = Log4Moz.repository.getLogger("Test");
  Log4Moz.repository.rootLogger.addAppender(new Log4Moz.DumpAppender());
  
  let keyBundle = ID.set("WeaveCryptoID",
      new SyncKeyBundle(PWDMGR_PASSPHRASE_REALM, "john@example.com", "a-bbbbb-ccccc-ddddd-eeeee-fffff"));
  
  




  
  log.info("Building storage keys...");
  let storage_keys = new CryptoWrapper("crypto", "keys");
  let default_key64 = Svc.Crypto.generateRandomKey();
  let default_hmac64 = Svc.Crypto.generateRandomKey();
  let bookmarks_key64 = Svc.Crypto.generateRandomKey();
  let bookmarks_hmac64 = Svc.Crypto.generateRandomKey();
  
  storage_keys.cleartext = {
    "default": [default_key64, default_hmac64],
    "collections": {"bookmarks": [bookmarks_key64, bookmarks_hmac64]},
  };
  storage_keys.modified = Date.now()/1000;
  storage_keys.id = "keys";
  
  log.info("Encrypting storage keys...");
  
  
  storage_keys.encrypt(keyBundle);
  
  
  do_check_true(null == storage_keys.cleartext);
  do_check_true(null != storage_keys.ciphertext);
  
  log.info("Updating CollectionKeys.");
  
  
  
  let payload = CollectionKeys.updateContents(keyBundle, storage_keys);
  
  _("CK: " + JSON.stringify(CollectionKeys._collections));
  
  
  let wbo = CollectionKeys.asWBO("crypto", "keys");
  
  _("WBO: " + JSON.stringify(wbo));
  
  
  do_check_eq(wbo.collection, "crypto");
  do_check_eq(wbo.id, "keys");
  do_check_eq(storage_keys.modified, wbo.modified);
  do_check_true(!!wbo.cleartext.default);
  do_check_keypair_eq(payload.default, wbo.cleartext.default);
  do_check_keypair_eq(payload.collections.bookmarks, wbo.cleartext.collections.bookmarks);
  
  do_check_true('bookmarks' in CollectionKeys._collections);
  do_check_false('tabs' in CollectionKeys._collections);
  
  



  let b1 = new BulkKeyBundle(null, "bookmarks");
  b1.keyPair = [bookmarks_key64, bookmarks_hmac64];
  let b2 = CollectionKeys.keyForCollection("bookmarks");
  do_check_keypair_eq(b1.keyPair, b2.keyPair);
  
  b1 = new BulkKeyBundle(null, "[default]");
  b1.keyPair = [default_key64, default_hmac64];
  b2 = CollectionKeys.keyForCollection(null);
  do_check_keypair_eq(b1.keyPair, b2.keyPair);
  
  


  let info_collections = {};
  do_check_true(CollectionKeys.updateNeeded(info_collections));
  info_collections["crypto"] = 5000;
  do_check_false(CollectionKeys.updateNeeded(info_collections));
  info_collections["crypto"] = 1 + (Date.now()/1000);              
  do_check_true(CollectionKeys.updateNeeded(info_collections));
  
  CollectionKeys._lastModified = null;
  do_check_true(CollectionKeys.updateNeeded({}));
}


function test_key_persistence() {
  _("Testing key persistence.");
  
  
  let k = new SyncKeyBundle(null, null, "abcdeabcdeabcdeabcdeabcdea");
  k.username = "john@example.com";
  ID.set("WeaveCryptoID", k);
  let id = ID.get("WeaveCryptoID");
  do_check_eq(k, id);
  id.persist();
  
  
  ID.del("WeaveCryptoID");
  k = id = null;
  
  
  id = new SyncKeyBundle();
  id.username = "john@example.com";
  
  
  do_check_eq(id.password, "abcdeabcdeabcdeabcdeabcdea");
  
  
  do_check_true(!!id.hmacKeyObject);
  do_check_true(!!id.hmacKey);
  do_check_true(!!id.encryptionKey);
}

function run_test() {
  test_keymanager();
  test_collections_manager();
  test_key_persistence();
  test_repeated_hmac();
  
  
  test_time_keyFromString(1000);
}
