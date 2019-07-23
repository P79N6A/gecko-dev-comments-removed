




































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);

function getTestDB()
{
  var db = dirSvc.get("CurProcD", Ci.nsIFile);
  db.append("test_storage.sqlite");
  return db;
}

function cleanup()
{
  
  var dbFile = getTestDB();
  if (dbFile.exists()) dbFile.remove(false);
}

function getService()
{
  return Cc["@mozilla.org/storage/service;1"].getService(Ci.mozIStorageService);
}

function getOpenedDatabase()
{
  return getService().openDatabase(getTestDB());
}

function createStatement(aSQL)
{
  return getOpenedDatabase().createStatement(aSQL);
}

cleanup();

