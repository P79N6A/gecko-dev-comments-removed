








































const NS_APP_USER_PROFILE_50_DIR = "ProfD";
const NS_APP_HISTORY_50_FILE = "UHist";

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

function LOG(aMsg) {
  aMsg = ("*** PLACES TESTS: " + aMsg);
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).
                                      logStringMessage(aMsg);
  print(aMsg);
}


var dirSvc = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
var profileDir = null;
try {
  profileDir = dirSvc.get(NS_APP_USER_PROFILE_50_DIR, Ci.nsIFile);
} catch (e) {}
if (!profileDir) {
  
  
  var provider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == NS_APP_USER_PROFILE_50_DIR) {
        return dirSvc.get("CurProcD", Ci.nsIFile);
      }
      if (prop == NS_APP_HISTORY_50_FILE) {
        var histFile = dirSvc.get("CurProcD", Ci.nsIFile);
        histFile.append("history.dat");
        return histFile;
      }
      throw Cr.NS_ERROR_FAILURE;
    },
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsIDirectoryServiceProvider) ||
          iid.equals(Ci.nsISupports)) {
        return this;
      }
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  };
  dirSvc.QueryInterface(Ci.nsIDirectoryService).registerProvider(provider);
}

var iosvc = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

function uri(spec) {
  return iosvc.newURI(spec, null, null);
}


function clearDB() {
  try {
    var file = dirSvc.get('ProfD', Ci.nsIFile);
    file.append("places.sqlite");
    if (file.exists())
      file.remove(false);
  } catch(ex) { dump("Exception: " + ex); }
}
clearDB();







function dump_table(aName)
{
  let db = DBConn()
  let stmt = db.createStatement("SELECT * FROM " + aName);

  dump("\n*** Printing data from " + aName + ":\n");
  let count = 0;
  while (stmt.executeStep()) {
    let columns = stmt.numEntries;

    if (count == 0) {
      
      for (let i = 0; i < columns; i++)
        dump(stmt.getColumnName(i) + "\t");
      dump("\n");
    }

    
    for (let i = 0; i < columns; i++) {
      switch (stmt.getTypeOfIndex(i)) {
        case Ci.mozIStorageValueArray.VALUE_TYPE_NULL:
          dump("NULL\t");
          break;
        case Ci.mozIStorageValueArray.VALUE_TYPE_INTEGER:
          dump(stmt.getInt64(i) + "\t");
          break;
        case Ci.mozIStorageValueArray.VALUE_TYPE_FLOAT:
          dump(stmt.getDouble(i) + "\t");
          break;
        case Ci.mozIStorageValueArray.VALUE_TYPE_TEXT:
          dump(stmt.getString(i) + "\t");
          break;
      }
    }
    dump("\n");

    count++;
  }
  dump("*** There were a total of " + count + " rows of data.\n\n");

  stmt.reset();
  stmt.finalize();
  stmt = null;
}















function new_test_bookmark_uri_event(aBookmarkId, aExpectedURI, aExpected, aFinish)
{
  let db = DBConn();
  let stmt = db.createStatement(
    "SELECT moz_places.url " +
    "FROM moz_bookmarks INNER JOIN moz_places " +
    "ON moz_bookmarks.fk = moz_places.id " +
    "WHERE moz_bookmarks.id = ?1"
  );
  stmt.bindInt64Parameter(0, aBookmarkId);

  if (aExpected) {
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.getUTF8String(0), aExpectedURI);
  }
  else {
    do_check_false(stmt.executeStep());
  }
  stmt.reset();
  stmt.finalize();
  stmt = null;

  if (aFinish)
    do_test_finished();
}















function new_test_visit_uri_event(aVisitId, aExpectedURI, aExpected, aFinish)
{
  let db = DBConn();
  let stmt = db.createStatement(
    "SELECT moz_places.url " +
    "FROM moz_historyvisits INNER JOIN moz_places " +
    "ON moz_historyvisits.place_id = moz_places.id " +
    "WHERE moz_historyvisits.id = ?1"
  );
  stmt.bindInt64Parameter(0, aVisitId);

  if (aExpected) {
    do_check_true(stmt.executeStep());
    do_check_eq(stmt.getUTF8String(0), aExpectedURI);
  }
  else {
    do_check_false(stmt.executeStep());
  }
  stmt.reset();
  stmt.finalize();
  stmt = null;

  if (aFinish)
    do_test_finished();
}





function DBConn()
{
  let db = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsPIPlacesDatabase).
           DBConnection;
  if (db.connectionReady)
    return db;

  
  let file = dirSvc.get('ProfD', Ci.nsIFile);
  file.append("places.sqlite");
  let storageService = Cc["@mozilla.org/storage/service;1"].
                       getService(Ci.mozIStorageService);
  try {
    var dbConn = storageService.openDatabase(file);
  } catch (ex) {
    return null;
  }
  return dbConn;
}




function flush_main_thread_events()
{
  let tm = Cc["@mozilla.org/thread-manager;1"].getService(Ci.nsIThreadManager);
  while (tm.mainThread.hasPendingEvents())
    tm.mainThread.processNextEvent(false);
}


function shutdownPlaces()
{
  const TOPIC_XPCOM_SHUTDOWN = "xpcom-shutdown";
  let hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsIObserver);
  hs.observe(null, TOPIC_XPCOM_SHUTDOWN, null);
  let sync = Cc["@mozilla.org/places/sync;1"].getService(Ci.nsIObserver);
  sync.observe(null, TOPIC_XPCOM_SHUTDOWN, null);
}
