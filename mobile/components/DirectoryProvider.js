



































const Cc = Components.classes;
const Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");





const NS_APP_CACHE_PARENT_DIR = "cachePDir";

function DirectoryProvider() {}

DirectoryProvider.prototype = {
  classDescription: "Directory Provider for special browser folders and files",
  contractID: "@mozilla.org/browser/directory-provider;1",
  classID: Components.ID("{ef0f7a87-c1ee-45a8-8d67-26f586e46a4b}"),
  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDirectoryServiceProvider]),

  _xpcom_categories: [
    { category: "xpcom-directory-providers", entry: "browser-directory-provider" }
  ],

  getFile: function(prop, persistent) {
    if (prop == NS_APP_CACHE_PARENT_DIR) {
      let dirsvc = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
      let profile = dirsvc.get("ProfD", Ci.nsIFile);

      let sysInfo = Cc["@mozilla.org/system-info;1"].getService(Ci.nsIPropertyBag2);
      let device = sysInfo.get("device");
      switch (device) {
#ifdef MOZ_PLATFORM_HILDON
        case "Nokia N900":
          return profile;
        
        case "Nokia N8xx":
          let folder = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
          folder.initWithPath("/media/mmc2/.mozilla/fennec");
          return folder;
#endif
        default:
          return profile;
      }
    }
    
    
    
    
    return null;
  }
};

function NSGetModule(aCompMgr, aFileSpec) {
  return XPCOMUtils.generateModule([DirectoryProvider]);
}

