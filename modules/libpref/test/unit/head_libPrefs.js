



































const NS_APP_USER_PROFILE_50_DIR = "ProfD";

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

var dirSvc = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);


var provider = {
  getFile: function(prop, persistent) {
    persistent.value = true;
    if (prop == NS_APP_USER_PROFILE_50_DIR)
      return dirSvc.get("CurProcD", Ci.nsIFile);
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
