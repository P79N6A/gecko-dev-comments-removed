




































const Ci = Components.interfaces;
const Cc = Components.classes;

const CURRENT_SCHEMA = 2;
const PR_HOURS = 60 * 60 * 1000000;

do_get_profile();

var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);

function getDBVersion(dbfile) {
    var ss = Cc["@mozilla.org/storage/service;1"].
             getService(Ci.mozIStorageService);
    var dbConnection = ss.openDatabase(dbfile);
    var version = dbConnection.schemaVersion;
    dbConnection.close();

    return version;
}

function cleanUpFormHist() {
  var formhistFile = dirSvc.get("ProfD", Ci.nsIFile);
  formhistFile.append("formhistory.dat");
  if (formhistFile.exists())
    formhistFile.remove(false);
}
cleanUpFormHist();
