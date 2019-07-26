



"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/PermissionSettings.jsm");

this.EXPORTED_SYMBOLS = ["PermissionsInstaller",
                         "expandPermissions",
                         "PermissionsTable",
                         "appendAccessToPermName"
                        ];
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








this.PermissionsTable =  { geolocation: {
                             app: PROMPT_ACTION,
                             privileged: PROMPT_ACTION,
                             certified: ALLOW_ACTION
                           },
                           camera: {
                             app: DENY_ACTION,
                             privileged: PROMPT_ACTION,
                             certified: ALLOW_ACTION
                           },
                           alarms: {
                             app: ALLOW_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "tcp-socket": {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "network-events": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           contacts: {
                             app: DENY_ACTION,
                             privileged: PROMPT_ACTION,
                             certified: ALLOW_ACTION,
                             access: ["read", "write", "create"]
                           },
                           "device-storage:apps": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION,
                             access: ["read"]
                           },
                           "device-storage:pictures": {
                             app: DENY_ACTION,
                             privileged: PROMPT_ACTION,
                             certified: ALLOW_ACTION,
                             access: ["read", "write", "create"]
                           },
                           "device-storage:videos": {
                             app: DENY_ACTION,
                             privileged: PROMPT_ACTION,
                             certified: ALLOW_ACTION,
                             access: ["read", "write", "create"]
                           },
                           "device-storage:music": {
                             app: DENY_ACTION,
                             privileged: PROMPT_ACTION,
                             certified: ALLOW_ACTION,
                             access: ["read", "write", "create"]
                           },
                           "device-storage:sdcard": {
                             app: DENY_ACTION,
                             privileged: PROMPT_ACTION,
                             certified: ALLOW_ACTION,
                             access: ["read", "write", "create"]
                           },
                           sms: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           telephony: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           browser: {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           bluetooth: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           mobileconnection: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           power: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           settings: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION,
                             access: ["read", "write"],
                             additional: ["indexedDB-chrome-settings"]
                           },
                           permissions: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           fmradio: {
                             app: ALLOW_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           attention: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "webapps-manage": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "backgroundservice": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "desktop-notification": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "networkstats-manage": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "wifi-manage": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "systemXHR": {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "voicemail": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "deprecated-hwvideo": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "idle": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "time": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "embed-apps": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "storage": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION,
                             substitute: [
                               "indexedDB-unlimited",
                               "offline-app",
                               "pin-app"
                             ]
                           },
                           "background-sensors": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           cellbroadcast: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           audio: {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION,
                             channels: ["normal", "content", "notification",
                               "alarm", "telephony", "ringer", "publicnotification"]
                           },
                         };









this.appendAccessToPermName = function appendAccessToPermName(aPermName, aAccess) {
  if (aAccess.length == 0) {
    return [aPermName];
  }
  return aAccess.map(function(aMode) {
    return aPermName + "-" + aMode;
  });
};










this.expandPermissions = function expandPermissions(aPermName, aAccess, aChannels) {
  if (!PermissionsTable[aPermName]) {
    Cu.reportError("PermissionsTable.jsm: expandPermissions: Unknown Permission: " + aPermName);
    dump("PermissionsTable.jsm: expandPermissions: Unknown Permission: " + aPermName);
    return [];
  }

  const tableEntry = PermissionsTable[aPermName];

  if (tableEntry.substitute && tableEntry.additional) {
    Cu.reportError("PermissionsTable.jsm: expandPermissions: Can't handle both 'substitute' " +
                   "and 'additional' entries for permission: " + aPermName);
    return [];
  }

  if (!aAccess && tableEntry.access ||
      aAccess && !tableEntry.access) {
    Cu.reportError("PermissionsTable.jsm: expandPermissions: Invalid Manifest : " +
                   aPermName + " " + aAccess + "\n");
    dump("PermissionsTable.jsm: expandPermissions: Invalid Manifest: " +
         aPermName + " " + aAccess + "\n");
    throw new Error("PermissionsTable.jsm: expandPermissions: Invalid Manifest: " +
                    aPermName + " " + aAccess + "\n");
  }

  let expandedPermNames = [];

  if (tableEntry.access && aAccess) {
    let requestedSuffixes = [];
    switch (aAccess) {
    case READONLY:
      requestedSuffixes.push("read");
      break;
    case CREATEONLY:
      requestedSuffixes.push("create");
      break;
    case READCREATE:
      requestedSuffixes.push("read", "create");
      break;
    case READWRITE:
      requestedSuffixes.push("read", "create", "write");
      break;
    default:
      return [];
    }

    let permArr = appendAccessToPermName(aPermName, requestedSuffixes);

    
    if (tableEntry.additional) {
      for each (let additional in tableEntry.additional) {
        permArr = permArr.concat(appendAccessToPermName(additional, requestedSuffixes));
      }
    }

    
    for (let idx in permArr) {
      let suffix = requestedSuffixes[idx % requestedSuffixes.length];
      if (tableEntry.access.indexOf(suffix) != -1) {
        expandedPermNames.push(permArr[idx]);
      }
    }
  } else if (tableEntry.substitute) {
    expandedPermNames = expandedPermNames.concat(tableEntry.substitute);
  } else if (tableEntry.channels) {
    if ("audio" == aPermName && aChannels) {
      let allowChannels = tableEntry.channels;

      for (let idx in aChannels) {
        let candidate = aChannels[idx];
        if (allowChannels.indexOf(candidate) == -1) {
          continue;
        }
        let permAttr = aPermName + "-channel-" + candidate;
        expandedPermNames.push(permAttr);
      }
    }
  } else {
    expandedPermNames.push(aPermName);
    
    if (tableEntry.additional) {
      expandedPermNames = expandedPermNames.concat(tableEntry.additional);
    }
  }

  return expandedPermNames;
};


let AllPossiblePermissions = [];
for (let permName in PermissionsTable) {
  if (PermissionsTable[permName].access) {
    AllPossiblePermissions =
      AllPossiblePermissions.concat(expandPermissions(permName, READWRITE));
  } else if (PermissionsTable[permName].channels) {
    AllPossiblePermissions =
      AllPossiblePermissions.concat(expandPermissions(permName, null, PermissionsTable[permName].channels));
  } else {
    AllPossiblePermissions =
      AllPossiblePermissions.concat(expandPermissions(permName));
  }
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
                                newManifest.permissions[permName].access,
                                newManifest.permissions[permName].channels);
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
                            newManifest.permissions[permName].access,
                            newManifest.permissions[permName].channels);
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
