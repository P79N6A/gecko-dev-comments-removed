



"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/PermissionsInstaller.jsm");
Cu.import("resource://gre/modules/PermissionsTable.jsm");
Cu.import("resource://gre/modules/PermissionSettings.jsm");

this.EXPORTED_SYMBOLS = ["SystemMessagePermissionsChecker",
                         "SystemMessagePermissionsTable"];

function debug(aStr) {
  
}






this.SystemMessagePermissionsTable = {
  "activity": { },
  "alarm": {
    "alarms": []
  },
  "bluetooth-dialer-command": {
    "telephony": []
  },
  "bluetooth-requestconfirmation": {
    "bluetooth": []
  },
  "bluetooth-requestpasskey": {
    "bluetooth": []
  },
  "bluetooth-requestpincode": {
    "bluetooth": []
  },
  "bluetooth-authorize": {
    "bluetooth": []
  },
  "bluetooth-cancel": {
    "bluetooth": []
  },
  "bluetooth-pairedstatuschanged": {
    "bluetooth": []
  },
  "bluetooth-hfp-status-changed": {
    "bluetooth": []
  },
  "bluetooth-opp-transfer-complete": {
    "bluetooth": []
  },
  "bluetooth-opp-update-progress": {
    "bluetooth": []
  },
  "bluetooth-opp-receiving-file-confirmation": {
    "bluetooth": []
  },
  "bluetooth-opp-transfer-start": {
    "bluetooth": []
  },
  "headset-button": { },
  "icc-stkcommand": {
    "settings": ["read", "write"]
  },
  "notification": {
    "desktop-notification": []
  },
  "push": {
  	"push": []
  },
  "push-register": {
  	"push": []
  },
  "sms-received": {
    "sms": []
  },
  "sms-sent": {
    "sms": []
  },
  "telephony-new-call": {
    "telephony": []
  },
  "telephony-call-ended": {
    "telephony": []
  },
  "ussd-received": {
    "mobileconnection": []
  }
};

this.SystemMessagePermissionsChecker = {
  











  getSystemMessagePermissions: function getSystemMessagePermissions(aSysMsgName) {
    debug("getSystemMessagePermissions(): aSysMsgName: " + aSysMsgName);

    let permNames = SystemMessagePermissionsTable[aSysMsgName];
    if (permNames === undefined) {
      debug("'" + aSysMsgName + "' is not associated with permissions. " +
            "Please add them to the SystemMessagePermissionsTable.");
      return null;
    }

    let object = { };
    for (let permName in permNames) {
      if (PermissionsTable[permName] === undefined) {
        debug("'" + permName + "' for '" + aSysMsgName + "' is invalid. " +
              "Please correct it in the SystemMessagePermissionsTable.");
        return null;
      }

      
      let access = permNames[permName];
      if (!access || !Array.isArray(access)) {
        debug("'" + permName + "' is not associated with access array. " +
              "Please correct it in the SystemMessagePermissionsTable.");
        return null;
      }
      object[permName] = appendAccessToPermName(permName, access);
    }
    return object
  },

  











  isSystemMessagePermittedToRegister:
    function isSystemMessagePermittedToRegister(aSysMsgName, aOrigin, aManifest) {
    debug("isSystemMessagePermittedToRegister(): " +
          "aSysMsgName: " + aSysMsgName + ", " +
          "aOrigin: " + aOrigin + ", " +
          "aManifest: " + JSON.stringify(aManifest));

    let permNames = this.getSystemMessagePermissions(aSysMsgName);
    if (permNames === null) {
      return false;
    }

    
    let appStatus;
    switch (AppsUtils.getAppManifestStatus(aManifest)) {
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
      throw new Error("SystemMessagePermissionsChecker.jsm: " +
                      "Cannot decide the app's status. Install cancelled.");
      break;
    }

    let newManifest = new ManifestHelper(aManifest, aOrigin);

    for (let permName in permNames) {
      
      if (!newManifest.permissions || !newManifest.permissions[permName]) {
        debug("'" + aSysMsgName + "' isn't permitted by '" + permName + "'. " +
              "Please add the permission for app: '" + aOrigin + "'.");
        return false;
      }
      let permValue = PermissionsTable[permName][appStatus];
      if (permValue != Ci.nsIPermissionManager.PROMPT_ACTION &&
          permValue != Ci.nsIPermissionManager.ALLOW_ACTION) {
        debug("'" + aSysMsgName + "' isn't permitted by '" + permName + "'. " +
              "Please add the permission for app: '" + aOrigin + "'.");
        return false;
      }

      
      
      let expandedPermNames =
        expandPermissions(permName,
                          newManifest.permissions[permName].access);

      let permNamesWithAccess = permNames[permName];

      
      for (let idx in permNamesWithAccess) {
        let index = expandedPermNames.indexOf(permNamesWithAccess[idx]);
        if (index == -1) {
          debug("'" + aSysMsgName + "' isn't permitted by '" + permName + "'. " +
                "Please add the permission for app: '" + aOrigin + "'.");
          return false;
        }
      }
    }

    
    return true;
  },

  











  isSystemMessagePermittedToSend:
    function isSystemMessagePermittedToSend(aSysMsgName, aPageURI, aManifestURL) {
    debug("isSystemMessagePermittedToSend(): " +
          "aSysMsgName: " + aSysMsgName + ", " +
          "aPageURI: " + aPageURI + ", " +
          "aManifestURL: " + aManifestURL);

    let permNames = this.getSystemMessagePermissions(aSysMsgName);
    if (permNames === null) {
      return false;
    }

    let pageURI = Services.io.newURI(aPageURI, null, null);
    for (let permName in permNames) {
      let permNamesWithAccess = permNames[permName];

      
      for (let idx in permNamesWithAccess) {
        if(PermissionSettingsModule.getPermission(permNamesWithAccess[idx],
                                                  aManifestURL,
                                                  pageURI.prePath,
                                                  false) != "allow") {
          debug("'" + aSysMsgName + "' isn't permitted by '" + permName + "'. " +
                "Please add the permission for app: '" + pageURI.prePath + "'.");
          return false;
        }
      }
    }

    
    return true;
  }
};
