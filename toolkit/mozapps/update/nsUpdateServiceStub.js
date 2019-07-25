




































*/
Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/FileUtils.jsm");

const Ci = Components.interfaces;

const DIR_UPDATES         = "updates";
const FILE_UPDATE_STATUS  = "update.status";

const KEY_APPDIR          = "XCurProcD";

#ifdef XP_WIN
#define USE_UPDROOT
#elifdef ANDROID
#define USE_UPDROOT
#endif

#ifdef USE_UPDROOT
const KEY_UPDROOT         = "UpdRootD";
#endif









function getUpdateDirNoCreate(pathArray) {
#ifdef USE_UPDROOT
  try {
    let dir = FileUtils.getDir(KEY_UPDROOT, pathArray, false);
    return dir;
  } catch (e) {
  }
#endif
  return FileUtils.getDir(KEY_APPDIR, pathArray, false);
}

function UpdateServiceStub() {
  let statusFile = getUpdateDirNoCreate([DIR_UPDATES, "0"]);
  statusFile.append(FILE_UPDATE_STATUS);
  
  if (statusFile.exists()) {
    let aus = Components.classes["@mozilla.org/updates/update-service;1"].
              getService(Ci.nsIApplicationUpdateService).
              QueryInterface(Ci.nsIObserver);
    aus.observe(null, "post-update-processing", "");
  }
}
UpdateServiceStub.prototype = {
  observe: function(){},
  classID: Components.ID("{e43b0010-04ba-4da6-b523-1f92580bc150}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver])
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([UpdateServiceStub]);
