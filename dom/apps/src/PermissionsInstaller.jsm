



"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/PermissionSettings.jsm");

var EXPORTED_SYMBOLS = ["PermissionsInstaller",
                        "expandPermissions",
                        "PermissionsTable",
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







function mapSuffixes(aPermName, aSuffixes)
{
  return aSuffixes.map(function(suf) { return aPermName + "-" + suf; });
}







const PermissionsTable = { "resource-lock": {
                             app: ALLOW_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           geolocation: {
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
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "device-storage:pictures": {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION,
                             access: ["read", "write", "create"]
                           },
                           "device-storage:videos": {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION,
                             access: ["read", "write", "create"]
                           },
                           "device-storage:music": {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
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
                           wifi: {
                             app: DENY_ACTION,
                             privileged: PROMPT_ACTION,
                             certified: ALLOW_ACTION
                           },
                           keyboard: {
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
                           push: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           settings: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION,
                             access: ["read", "write"]
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
                           "mozBluetooth": {
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
                             privileged: DENY_ACTION,
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
                             certified: ALLOW_ACTION
                           },
                           "background-sensors": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                         };


let AllPossiblePermissions = [];
for (let permName in PermissionsTable) {
  if (PermissionsTable[permName].access) {
    AllPossiblePermissions =
      AllPossiblePermissions.concat(mapSuffixes(permName,
                                    PermissionsTable[permName].access));
  }
  else {
    AllPossiblePermissions.push(permName);
  }
}









function expandPermissions(aPermName, aAccess) {
  if (!PermissionsTable[aPermName]) {
    Cu.reportError("PermissionsTable.jsm: expandPermissions: Unknown Permission: " + aPermName);
    throw new Error("PermissionsTable.jsm: expandPermissions: Unknown Permission: " + aPermName);
  }











  if (!PermissionsTable[aPermName].access || !aAccess) {
    return [aPermName];
  }

  let requestedSuffixes = [];
  switch(aAccess) {
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

  let permArr = mapSuffixes(aPermName, requestedSuffixes);

  let expandedPerms = [aPermName];
  for (let idx in permArr) {
    if (PermissionsTable[aPermName].access.indexOf(requestedSuffixes[idx]) != -1) {
      expandedPerms.push(permArr[idx]);
    }
  }
  return expandedPerms;
}

let PermissionsInstaller = {











  installPermissions: function installPermissions(aApp, aIsReinstall, aOnError) {
    try {
      let newManifest = new ManifestHelper(aApp.manifest, aApp.origin);
      if (!newManifest.permissions && !aIsReinstall) {
        return;
      }

      if (aIsReinstall) {
        
        

        if (newManifest.permissions) {
          
          let newPerms = [];
          for (let perm in newManifest.permissions) {
            let _perms = expandPermissions(perm,
                                           newManifest.permissions[perm].access);
            newPerms = newPerms.concat(_perms);
          }

          for (let idx in AllPossiblePermissions) {
            let index = newPerms.indexOf(AllPossiblePermissions[idx]);
            if (index == -1) {
              
              let _perm = PermissionSettingsModule.getPermission(AllPossiblePermissions[idx],
                                           aApp.manifestURL,
                                           aApp.origin,
                                           false);
              if (_perm == "unknown" || _perm == "deny") {
                
                continue;
              }
              
              
              this._setPermission(AllPossiblePermissions[idx], "unknown", aApp);
            }
          }
        }
      }

      let installPermType;
      
      switch (AppsUtils.getAppManifestStatus(aApp.manifest)) {
      case Ci.nsIPrincipal.APP_STATUS_CERTIFIED:
        installPermType = "certified";
        break;
      case Ci.nsIPrincipal.APP_STATUS_PRIVILEGED:
        installPermType = "privileged";
        break;
      case Ci.nsIPrincipal.APP_STATUS_INSTALLED:
        installPermType = "app";
        break;
      default:
        
        throw new Error("PermissionsInstaller.jsm: Cannot determine app type, install cancelled");
      }

      for (let permName in newManifest.permissions) {
        if (!PermissionsTable[permName]) {
          throw new Error("PermissionsInstaller.jsm: '" + permName + "'" +
                         " is not a valid Webapps permission type. Aborting Webapp installation");
          return;
        }

        let perms = expandPermissions(permName,
                                      newManifest.permissions[permName].access);
        for (let idx in perms) {
          let perm = PermissionsTable[permName][installPermType];
          let permValue = PERM_TO_STRING[perm];
          this._setPermission(perms[idx], permValue, aApp);
        }
      }
    }
    catch (ex) {
      debug("Caught webapps install permissions error");
      Cu.reportError(ex);
      if (aOnError) {
        aOnError();
      }
    }
  },

  










  _setPermission: function setPermission(aPerm, aValue, aApp) {
    if (aPerm != "storage") {
      PermissionSettingsModule.addPermission({
        type: aPerm,
        origin: aApp.origin,
        manifestURL: aApp.manifestURL,
        value: aValue,
        browserFlag: false
      });
      return;
    }

    ["indexedDB-unlimited", "offline-app", "pin-app"].forEach(
      function(aName) {
        PermissionSettingsModule.addPermission({
          type: aName,
          origin: aApp.origin,
          manifestURL: aApp.manifestURL,
          value: aValue,
          browserFlag: false
        });
      }
    );
  }
}
