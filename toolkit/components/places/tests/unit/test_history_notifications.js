






































const NS_PLACES_INIT_COMPLETE_TOPIC = "places-init-complete";
const NS_PLACES_DATABASE_LOCKED_TOPIC = "places-database-locked";

function run_test() {
  do_test_pending();

  
  var os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
  var observer = {
    _lockedNotificationReceived: false,
    observe: function thn_observe(aSubject, aTopic, aData)
    {
      switch (aTopic) {
        case NS_PLACES_INIT_COMPLETE_TOPIC:
          do_check_true(this._lockedNotificationReceived);
          os.removeObserver(this, NS_PLACES_INIT_COMPLETE_TOPIC);
          os.removeObserver(this, NS_PLACES_DATABASE_LOCKED_TOPIC);
          do_test_finished();
          break;
        case NS_PLACES_DATABASE_LOCKED_TOPIC:
          if (this._lockedNotificationReceived)
            do_throw("Locked notification should be observed only one time");
          this._lockedNotificationReceived = true;
          break;
      }
    }
  };
  os.addObserver(observer, NS_PLACES_INIT_COMPLETE_TOPIC, false);
  os.addObserver(observer, NS_PLACES_DATABASE_LOCKED_TOPIC, false);

  
  var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
               getService(Ci.nsIProperties);
  var db = dirSvc.get('ProfD', Ci.nsIFile);
  db.append("places.sqlite");
  var storage = Cc["@mozilla.org/storage/service;1"].
                getService(Ci.mozIStorageService);
  var dbConn = storage.openUnsharedDatabase(db);
  do_check_true(db.exists());

  
  dbConn.executeSimpleSQL("PRAGMA locking_mode = EXCLUSIVE");
  
  dbConn.executeSimpleSQL("PRAGMA USER_VERSION = 1");

  
  try {
    var hs1 = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);
    do_throw("Creating an instance of history service on a locked db should throw");
  } catch (ex) {}

  
  dbConn.close();
  if (db.exists()) {
    try {
      db.remove(false);
    } catch(e) { dump("Unable to remove dummy places.sqlite"); }
  }

  
  try {
    var hs2 = Cc["@mozilla.org/browser/nav-history-service;1"].
              getService(Ci.nsINavHistoryService);
  } catch (ex) {
    do_throw("Creating an instance of history service on a not locked db should not throw");
  }
}
