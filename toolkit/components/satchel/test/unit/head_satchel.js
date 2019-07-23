



































 
const Ci = Components.interfaces;
const Cc = Components.classes;

const CURRENT_SCHEMA = 2;
const PR_HOURS = 60 * 60 * 1000000;


var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);

var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);
var profileDir = null;
try {
  profileDir = dirSvc.get("ProfD", Ci.nsIFile);
} catch (e) { }
if (!profileDir) {
  
  
  var provider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == "ProfD") {
        return dirSvc.get("CurProcD", Ci.nsILocalFile);
      }
      print("*** Throwing trying to get " + prop);
      throw Cr.NS_ERROR_FAILURE;
    },
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsIDirectoryProvider) ||
          iid.equals(Ci.nsISupports)) {
        return this;
      }
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  };
  dirSvc.QueryInterface(Ci.nsIDirectoryService).registerProvider(provider);
}

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
