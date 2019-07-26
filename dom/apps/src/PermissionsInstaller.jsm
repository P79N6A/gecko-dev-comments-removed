



"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/PermissionSettings.jsm");
Cu.import("resource://gre/modules/PermissionsTable.jsm");

this.EXPORTED_SYMBOLS = ["PermissionsInstaller"];
const UNKNOWN_ACTION = Ci.nsIPermissionManager.UNKNOWN_ACTION;
const ALLOW_ACTION = Ci.nsIPermissionManager.ALLOW_ACTION;
const DENY_ACTION = Ci.nsIPermissionManager.DENY_ACTION;
const PROMPT_ACTION = Ci.nsIPermissionManager.PROMPT_ACTION;


const READONLY = "readonly";
const CREATEONLY = "createonly";
const READCREATE = "readcreate";
const READWRITE = "readwrite";

const PERM_TO_STRING = ["unknown", "allow", "deny", "prompt"];

function debug(aMsg) {
  
}


let AllPossiblePermissions = [];
for (let permName in PermissionsTable) {
  let expandedPermNames = [];
  if (PermissionsTable[permName].access) {
    expandedPermNames = expandPermissions(permName, READWRITE);
  } else {
    expandedPermNames = expandPermissions(permName);
  }
  AllPossiblePermissions = AllPossiblePermissions.concat(expandedPermNames);
}

this.PermissionsInstaller = {
  










  installPermissions: function installPermissions(aApp, aIsReinstall, aOnError) {
    try {
      let newManifest = new ManifestHelper(aApp.manifest, aApp.origin);
      if (!newManifest.permissions && !aIsReinstall) {
        return;
      }

      if (aIsReinstall) {
        
        

        if (newManifest.permissions) {
          
          let newPermNames = [];
          for (let permName in newManifest.permissions) {
            let expandedPermNames =
              expandPermissions(permName,
                                newManifest.permissions[permName].access);
            newPermNames = newPermNames.concat(expandedPermNames);
          }

          for (let idx in AllPossiblePermissions) {
            let permName = AllPossiblePermissions[idx];
            let index = newPermNames.indexOf(permName);
            if (index == -1) {
              
              let permValue =
                PermissionSettingsModule.getPermission(permName,
                                                       aApp.manifestURL,
                                                       aApp.origin,
                                                       false);
              if (permValue == "unknown" || permValue == "deny") {
                
                continue;
              }
              
              
              this._setPermission(permName, "unknown", aApp);
            }
          }
        }
      }

      
      let appStatus;
      switch (AppsUtils.getAppManifestStatus(aApp.manifest)) {
      case Ci.nsIPrincipal.APP_STATUS_CERTIFIED:
        appStatus = "certified";
        break;
      case Ci.nsIPrincipal.APP_STATUS_PRIVILEGED:
        appStatus = "privileged";
        break;
      case Ci.nsIPrincipal.APP_STATUS_INSTALLED:
        appStatus = "app";
        break;
      default:
        
        throw new Error("PermissionsInstaller.jsm: " +
                        "Cannot determine the app's status. Install cancelled.");
        break;
      }

      for (let permName in newManifest.permissions) {
        if (!PermissionsTable[permName]) {
          Cu.reportError("PermissionsInstaller.jsm: '" + permName + "'" +
                         " is not a valid Webapps permission name.");
          dump("PermissionsInstaller.jsm: '" + permName + "'" +
               " is not a valid Webapps permission name.");
          continue;
        }

        let expandedPermNames =
          expandPermissions(permName,
                            newManifest.permissions[permName].access);
        for (let idx in expandedPermNames) {
          this._setPermission(expandedPermNames[idx],
                              PERM_TO_STRING[PermissionsTable[permName][appStatus]],
                              aApp);
        }
      }
    }
    catch (ex) {
      dump("Caught webapps install permissions error for " + aApp.origin);
      Cu.reportError(ex);
      if (aOnError) {
        aOnError();
      }
    }
  },

  










  _setPermission: function setPermission(aPermName, aPermValue, aApp) {
    PermissionSettingsModule.addPermission({
      type: aPermName,
      origin: aApp.origin,
      manifestURL: aApp.manifestURL,
      value: aPermValue,
      browserFlag: false
    });
  }
};
