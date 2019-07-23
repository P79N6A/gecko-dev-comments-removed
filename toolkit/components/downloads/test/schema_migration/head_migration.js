



































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

do_get_profile();

var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);

function importDatabaseFile(aFName)
{
  var file = do_get_file(aFName);
  var newFile = dirSvc.get("ProfD", Ci.nsIFile);
  file.copyTo(newFile, "downloads.sqlite");
}

function cleanup()
{
  
  var dbFile = dirSvc.get("ProfD", Ci.nsIFile);
  dbFile.append("downloads.sqlite");
  if (dbFile.exists())
    try { dbFile.remove(true); } catch(e) {  }
}

cleanup();

