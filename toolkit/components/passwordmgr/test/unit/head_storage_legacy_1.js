

const NS_APP_USER_PROFILE_50_DIR = "ProfD";
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;


var dirSvc = Cc["@mozilla.org/file/directory_service;1"]
                    .getService(Ci.nsIProperties);

try {
  var profileDir = dirSvc.get(NS_APP_USER_PROFILE_50_DIR, Ci.nsIFile);
} catch (e) { }

if (!profileDir) {
  
  
  var provider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == NS_APP_USER_PROFILE_50_DIR) {
        return dirSvc.get("CurProcD", Ci.nsIFile);
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



var keydb = do_get_file("toolkit/components/passwordmgr/test/unit/key3.db");
profileDir = dirSvc.get(NS_APP_USER_PROFILE_50_DIR, Ci.nsIFile);




try {
    var oldfile = Cc["@mozilla.org/file/local;1"]
                        .createInstance(Ci.nsILocalFile);
    oldfile.initWithPath(profileDir.path + "/key3.db");
    if (oldfile.exists())
        oldfile.remove(false);
} catch(e) { }


keydb.copyTo(profileDir, "key3.db");
