



"use strict";

this.EXPORTED_SYMBOLS = ["ResetProfile"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;
#expand const MOZ_APP_NAME = "__MOZ_APP_NAME__";
#expand const MOZ_BUILD_APP = "__MOZ_BUILD_APP__";

Cu.import("resource://gre/modules/Services.jsm");

this.ResetProfile = {
  




  resetSupported: function() {
    let profileService = Cc["@mozilla.org/toolkit/profile-service;1"].
                         getService(Ci.nsIToolkitProfileService);
    let currentProfileDir = Services.dirsvc.get("ProfD", Ci.nsIFile);

    
    try {
      return currentProfileDir.equals(profileService.selectedProfile.rootDir) &&
        ("@mozilla.org/profile/migrator;1?app=" + MOZ_BUILD_APP + "&type=" + MOZ_APP_NAME in Cc);
    } catch (e) {
      
      Cu.reportError(e);
    }
    return false;
  },

  getMigratedData: function() {
    Cu.import("resource:///modules/MigrationUtils.jsm");

    
    const MIGRATED_TYPES = [
      128,
      4,  
      16, 
      8,  
      2,  
    ];

    
    let dataTypes = [];
    for (let itemID of MIGRATED_TYPES) {
      try {
        let typeName = MigrationUtils.getLocalizedString(itemID + "_" + MOZ_APP_NAME);
        dataTypes.push(typeName);
      } catch (x) {
        
        Cu.reportError(x);
      }
    }
    return dataTypes;
  },

  


  openConfirmationDialog: function(window) {
    
    let params = {
      reset: false,
    };
    window.openDialog("chrome://global/content/resetProfile.xul", null,
                      "chrome,modal,centerscreen,titlebar,dialog=yes", params);
    if (!params.reset)
      return;

    
    let env = Cc["@mozilla.org/process/environment;1"]
                .getService(Ci.nsIEnvironment);
    env.set("MOZ_RESET_PROFILE_RESTART", "1");

    let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
    appStartup.quit(Ci.nsIAppStartup.eForceQuit | Ci.nsIAppStartup.eRestart);
  },
};
