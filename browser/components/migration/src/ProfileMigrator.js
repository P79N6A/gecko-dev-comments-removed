



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/MigrationUtils.jsm");

function ProfileMigrator() {
}

ProfileMigrator.prototype = {
  migrate: function PM_migrate(aStartup, aKey) {
    let key = aKey;
    let skipSourcePage = false;
    if (key.length > 0) {
      if (!MigrationUtils.getMigrator(key)) {
        Cu.reportError("Invalid migrator key specified or source does not exist.");
        return;   
      }

      
      
      skipSourcePage = true;
    }
    else {
      key = this._getDefaultMigrator();
    }

    if (!key)
      return;

    MigrationUtils.showMigrationWizard(null, aStartup, key, skipSourcePage);
  },

  
  
  
  _PLATFORM_FALLBACK_LIST:
#ifdef XP_WIN
     ["ie", "chrome", ],
#elifdef XP_MACOSX
     ["safari", "chrome"],
#else
     ["chrome"],
#endif

  _getDefaultMigrator: function PM__getDefaultMigrator() {
    let migratorsOrdered = Array.slice(this._PLATFORM_FALLBACK_LIST);
    let defaultBrowser = "";
#ifdef XP_WIN
    try {
      const REG_KEY = "SOFTWARE\\Classes\\HTTP\\shell\\open\\command";
      let regKey = Cc["@mozilla.org/windows-registry-key;1"].
                   createInstance(Ci.nsIWindowsRegKey);
      regKey.open(regKey.ROOT_KEY_LOCAL_MACHINE, REG_KEY,
                  regKey.ACCESS_READ);
      let value = regKey.readStringValue("").toLowerCase();      
      let pathMatches = value.match(/^"?(.+?\.exe)"?/);
      if (!pathMatches) {
        throw new Error("Could not extract path from " +
                        REG_KEY + "(" + value + ")");
      }
 
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      let file = FileUtils.File(pathMatches[1])
                          .QueryInterface(Ci.nsILocalFileWin);
      switch (file.getVersionInfoField("InternalName").toLowerCase()) {
        case "iexplore":
          defaultBrowser = "ie";
          break;
        case "chrome":
          defaultBrowser = "chrome";
          break;
      }
    }
    catch (ex) {
      Cu.reportError("Could not retrieve default browser: " + ex);
    }
#endif

    
    
    
    if (defaultBrowser)
      migratorsOrdered.sort(function(a, b) b == defaultBrowser ? 1 : 0);

    for (let i = 0; i < migratorsOrdered.length; i++) {
      let migrator = MigrationUtils.getMigrator(migratorsOrdered[i]);
      if (migrator)
        return migratorsOrdered[i];
    }

    return "";
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIProfileMigrator]),
  classDescription: "Profile Migrator",
  contractID: "@mozilla.org/toolkit/profile-migrator;1",
  classID: Components.ID("6F8BB968-C14F-4D6F-9733-6C6737B35DCE")
};

let NSGetFactory = XPCOMUtils.generateNSGetFactory([ProfileMigrator]);
