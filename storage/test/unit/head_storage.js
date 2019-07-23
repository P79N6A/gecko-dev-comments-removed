




































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

do_get_profile();
var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);

function getTestDB()
{
  var db = dirSvc.get("ProfD", Ci.nsIFile);
  db.append("test_storage.sqlite");
  return db;
}




function getCorruptDB()
{
  return do_get_file("corruptDB.sqlite");
}

function cleanup()
{
  
  print("*** Storage Tests: Trying to close!");
  getOpenedDatabase().close();

  
  
  gDBConn = null;

  
  print("*** Storage Tests: Trying to remove file!");
  var dbFile = getTestDB();
  if (dbFile.exists())
    try { dbFile.remove(false); } catch(e) {  }
}

function getService()
{
  return Cc["@mozilla.org/storage/service;1"].getService(Ci.mozIStorageService);
}

var gDBConn = null;











function getOpenedDatabase(unshared)
{
  if (!gDBConn) {
    gDBConn = getService()
              [unshared ? "openUnsharedDatabase" : "openDatabase"]
              (getTestDB());
  }
  return gDBConn;
}








function getDatabase(aFile)
{
  return getService().openDatabase(aFile);
}

function createStatement(aSQL)
{
  return getOpenedDatabase().createStatement(aSQL);
}

cleanup();

