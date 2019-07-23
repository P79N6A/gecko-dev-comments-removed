



































 
const Ci = Components.interfaces;
const Cc = Components.classes;
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

function cleanUpFormHist() {
  var formhistFile = dirSvc.get("ProfD", Ci.nsIFile);
  formhistFile.append("formhistory.dat");
  if (formhistFile.exists())
    formhistFile.remove(false);
}
cleanUpFormHist();
