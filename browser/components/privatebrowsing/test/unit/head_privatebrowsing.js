




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

const kPrivateBrowsingNotification = "private-browsing";
const kPrivateBrowsingCancelVoteNotification = "private-browsing-cancel-vote";
const kEnter = "enter";
const kExit = "exit";

const NS_APP_USER_PROFILE_50_DIR = "ProfD";
const NS_APP_HISTORY_50_FILE = "UHist";

function LOG(aMsg) {
  aMsg = ("*** PRIVATEBROWSING TESTS: " + aMsg);
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).
                                      logStringMessage(aMsg);
  print(aMsg);
}


var dirSvc = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
var profileDir = null;
try {
  profileDir = dirSvc.get(NS_APP_USER_PROFILE_50_DIR, Ci.nsIFile);
} catch (e) {}
if (!profileDir) {
  
  
  var provider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == NS_APP_USER_PROFILE_50_DIR) {
        return dirSvc.get("CurProcD", Ci.nsIFile);
      }
      if (prop == NS_APP_HISTORY_50_FILE) {
        var histFile = dirSvc.get("CurProcD", Ci.nsIFile);
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




function cleanUp()
{
  let files = [
    "downloads.sqlite",
    "places.sqlite",
    "cookies.sqlite",
    "signons.sqlite",
    "permissions.sqlite"
  ];

  for (let i = 0; i < files.length; i++) {
    let file = dirSvc.get("ProfD", Ci.nsIFile);
    file.append(files[i]);
    if (file.exists())
      file.remove(false);
  }
}
cleanUp();
