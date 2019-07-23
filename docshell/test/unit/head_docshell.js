




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);
var profileDir = dirSvc.get("CurProcD", Ci.nsILocalFile);
profileDir.append("test_docshell_profile");



var provider = {
  getFile: function(prop, persistent) {
    persistent.value = true;
    if (prop == "ProfD") {
      var retVal = dirSvc.get("CurProcD", Ci.nsILocalFile);
      retVal.append("test_docshell_profile");
      if (!retVal.exists())
        retVal.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
      return retVal;
    }
    if (prop == "UHist") {
      var retVal = dirSvc.get("CurProcD", Ci.nsILocalFile);
      retVal.append("test_docshell_profile");
      if (!retVal.exists())
        retVal.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
      retVal.append("history.dat");
      return retVal;
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

function cleanup()
{
  
  try {
    if (profileDir.exists())
      profileDir.remove(true);
  } catch (e) {
    
    
    
    
  }
}



cleanup();


profileDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
