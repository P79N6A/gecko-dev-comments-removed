





































const NS_APP_USER_PROFILE_50_DIR = "ProfD";
const NS_APP_HISTORY_50_FILE = "UHist";

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;


let dirSvc = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
let profileDir = null;
try {
  profileDir = dirSvc.get(NS_APP_USER_PROFILE_50_DIR, Ci.nsIFile);
} catch (e) {}
if (!profileDir) {
  
  
  let provider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == NS_APP_USER_PROFILE_50_DIR) {
        return dirSvc.get("CurProcD", Ci.nsIFile);
      }
      if (prop == NS_APP_HISTORY_50_FILE) {
        let histFile = dirSvc.get("CurProcD", Ci.nsIFile);
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


function clearDB() {
  try {
    let file = dirSvc.get('ProfD', Ci.nsIFile);
    file.append("places.sqlite");
    if (file.exists())
      file.remove(false);
  } catch(ex) { dump("Exception: " + ex); }
}
clearDB();
