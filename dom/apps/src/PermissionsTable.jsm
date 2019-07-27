



"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

this.EXPORTED_SYMBOLS = [
  "PermissionsTable",
  "PermissionsReverseTable",
  "expandPermissions",
  "appendAccessToPermName",
  "isExplicitInPermissionsTable",
  "AllPossiblePermissions"
];


const READONLY = "readonly";
const CREATEONLY = "createonly";
const READCREATE = "readcreate";
const READWRITE = "readwrite";

const UNKNOWN_ACTION = Ci.nsIPermissionManager.UNKNOWN_ACTION;
const ALLOW_ACTION = Ci.nsIPermissionManager.ALLOW_ACTION;
const DENY_ACTION = Ci.nsIPermissionManager.DENY_ACTION;
const PROMPT_ACTION = Ci.nsIPermissionManager.PROMPT_ACTION;







this.PermissionsTable =  { geolocation: {
                             app: PROMPT_ACTION,
                             privileged: PROMPT_ACTION,
                             certified: PROMPT_ACTION
                           },
                           "geolocation-noprompt": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION,
                             substitute: ["geolocation"]
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
                           "udp-socket": {
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
                           "device-storage:crashes": {
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
                           mobilenetwork: {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           power: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           push: {
                            app: ALLOW_ACTION,
                            privileged: ALLOW_ACTION,
                            certified: ALLOW_ACTION
                           },
                           settings: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION,
                             access: ["read", "write"],
                             additional: ["indexedDB-chrome-settings", "settings-api"]
                           },
                           
                           
                           
                           "settings-clear": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: DENY_ACTION,
                             additional: ["indexedDB-chrome-settings", "settings-api"]
                           },
                           permissions: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           phonenumberservice: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           fmradio: {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           attention: {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "moz-attention": {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION,
                             substitute: ["attention"]
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
                             app: ALLOW_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "networkstats-manage": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "resourcestats-manage": {
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
                           "embed-widgets": {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "storage": {
                             app: ALLOW_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION,
                             substitute: [
                               "indexedDB-unlimited",
                               "default-persistent-storage"
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
                           "audio-channel-normal": {
                             app: ALLOW_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "audio-channel-content": {
                             app: ALLOW_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "audio-channel-notification": {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "audio-channel-alarm": {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "audio-channel-telephony": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "moz-audio-channel-telephony": {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION,
                             substitute: ["audio-channel-telephony"]
                           },
                           "audio-channel-ringer": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "moz-audio-channel-ringer": {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION,
                             substitute: ["audio-channel-ringer"]
                           },
                           "audio-channel-publicnotification": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "open-remote-window": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "input": {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "input-manage": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "wappush": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "audio-capture": {
                             app: PROMPT_ACTION,
                             privileged: PROMPT_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "nfc": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION,
                             access: ["read", "write"]
                           },
                           "nfc-manager": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "nfc-hci-events": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "speaker-control": {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "downloads": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "video-capture": {
                             app: PROMPT_ACTION,
                             privileged: PROMPT_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "feature-detection": {
                             app: DENY_ACTION,
                             privileged: ALLOW_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "mobileid": {
                             app: DENY_ACTION,
                             privileged: PROMPT_ACTION,
                             certified: PROMPT_ACTION
                           },
                           
                           
                           
                           "test-permission": {
                             app: PROMPT_ACTION,
                             privileged: PROMPT_ACTION,
                             certified: ALLOW_ACTION,
                             access: ["read", "write", "create"]
                           },
                           "firefox-accounts": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           },
                           "moz-firefox-accounts": {
                             app: DENY_ACTION,
                             privileged: PROMPT_ACTION,
                             certified: ALLOW_ACTION,
                             substitute: ["firefox-accounts"]
                             },
                           "themeable": {
                             app: DENY_ACTION,
                             privileged: DENY_ACTION,
                             certified: ALLOW_ACTION
                           }
                         };









this.appendAccessToPermName = function appendAccessToPermName(aPermName, aAccess) {
  if (aAccess.length == 0) {
    return [aPermName];
  }
  return aAccess.map(function(aMode) {
    return aPermName + "-" + aMode;
  });
};









this.expandPermissions = function expandPermissions(aPermName, aAccess) {
  if (!PermissionsTable[aPermName]) {
    let errorMsg =
      "PermissionsTable.jsm: expandPermissions: Unknown Permission: " + aPermName;
    Cu.reportError(errorMsg);
    dump(errorMsg);
    return [];
  }

  const tableEntry = PermissionsTable[aPermName];

  if (tableEntry.substitute && tableEntry.additional) {
    let errorMsg =
      "PermissionsTable.jsm: expandPermissions: Can't handle both 'substitute' " +
      "and 'additional' entries for permission: " + aPermName;
    Cu.reportError(errorMsg);
    dump(errorMsg);
    return [];
  }

  if (!aAccess && tableEntry.access ||
      aAccess && !tableEntry.access) {
    let errorMsg =
      "PermissionsTable.jsm: expandPermissions: Invalid access for permission " +
      aPermName + ": " + aAccess + "\n";
    Cu.reportError(errorMsg);
    dump(errorMsg);
    return [];
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
  } else {
    expandedPermNames.push(aPermName);
    
    if (tableEntry.additional) {
      expandedPermNames = expandedPermNames.concat(tableEntry.additional);
    }
  }

  return expandedPermNames;
};

this.PermissionsReverseTable = {};
this.AllPossiblePermissions = [];

(function () {
  
  
  
  
  
  
  
  
  
  
  for (let permName in PermissionsTable) {
    let permAliases = [];
    if (PermissionsTable[permName].access) {
      permAliases = expandPermissions(permName, "readwrite");
    } else if (!PermissionsTable[permName].substitute) {
      permAliases = expandPermissions(permName);
    }
    for (let i = 0; i < permAliases.length; i++) {
      PermissionsReverseTable[permAliases[i]] = permName;
      AllPossiblePermissions.push(permAliases[i]);
    }
  }
  AllPossiblePermissions =
    AllPossiblePermissions.concat(["offline-app", "pin-app"]);
})();

this.isExplicitInPermissionsTable = function(aPermName, aIntStatus) {

  
  let appStatus;
  switch (aIntStatus) {
    case Ci.nsIPrincipal.APP_STATUS_CERTIFIED:
      appStatus = "certified";
      break;
    case Ci.nsIPrincipal.APP_STATUS_PRIVILEGED:
      appStatus = "privileged";
      break;
    default: 
      appStatus = "app";
      break;
  }

  let realPerm = PermissionsReverseTable[aPermName];

  if (realPerm) {
    return (PermissionsTable[realPerm][appStatus] ==
            Ci.nsIPermissionManager.PROMPT_ACTION);
  } else {
    return false;
  }
}
