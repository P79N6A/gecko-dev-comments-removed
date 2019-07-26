


MARIONETTE_CONTEXT = "chrome";

let Promise = Cu.import("resource://gre/modules/Promise.jsm").Promise;





let MMDB;






function newMobileMessageDB() {
  if (!MMDB) {
    MMDB = Cu.import("resource://gre/modules/MobileMessageDB.jsm", {});
    is(typeof MMDB.MobileMessageDB, "function", "MMDB.MobileMessageDB");
  }

  let mmdb = new MMDB.MobileMessageDB();
  ok(mmdb, "MobileMessageDB instance");
  return mmdb;
}


















function initMobileMessageDB(aMmdb, aDbName, aDbVersion) {
  let deferred = Promise.defer();

  aMmdb.init(aDbName, aDbVersion, function(aError) {
    if (aError) {
      deferred.reject(aMmdb);
    } else {
      deferred.resolve(aMmdb);
    }
  });

  return deferred.promise;
}









function closeMobileMessageDB(aMmdb) {
  aMmdb.close();
  return aMmdb;
}


let _uuidGenerator;






function newUUID() {
  if (!_uuidGenerator) {
    _uuidGenerator = Cc["@mozilla.org/uuid-generator;1"]
                     .getService(Ci.nsIUUIDGenerator);
    ok(_uuidGenerator, "uuidGenerator");
  }

  return _uuidGenerator.generateUUID().toString();
}




function cleanUp() {
  
  ok(true, "permissions flushed");

  finish();
}









function startTestBase(aTestCaseMain) {
  Promise.resolve()
    .then(aTestCaseMain)
    .then(null, function() {
      ok(false, 'promise rejects during test.');
    })
    .then(cleanUp);
}
