



"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/PermissionsInstaller.jsm");
Cu.import("resource://gre/modules/PermissionsTable.jsm");
Cu.import("resource://gre/modules/PermissionSettings.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "dataStoreService",
                                   "@mozilla.org/datastore-service;1",
                                   "nsIDataStoreService");

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
  "bluetooth-cancel": {
    "bluetooth": []
  },
  "bluetooth-hid-status-changed": {
    "bluetooth": []
  },
  "bluetooth-pairing-request": {
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
  "cellbroadcast-received": {
    "cellbroadcast": []
  },
  "connection": { },
  "captive-portal": {
    "wifi-manage": []
  },
  "dummy-system-message": { }, 
  "dummy-system-message2": { }, 
  "headset-button": { },
  "icc-stkcommand": {
    "settings": ["read", "write"]
  },
  "media-button": { },
  "networkstats-alarm": {
    "networkstats-manage": []
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
  "request-sync": { },
  "sms-delivery-success": {
    "sms": []
  },
  "sms-delivery-error": {
    "sms": []
  },
  "sms-read-success": {
    "sms": []
  },
  "sms-received": {
    "sms": []
  },
  "sms-sent": {
    "sms": []
  },
  "sms-failed": {
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
  },
  "wappush-received": {
    "wappush": []
  },
  "cdma-info-rec-received": {
    "mobileconnection": []
  },
  "nfc-hci-event-transaction": {
    "nfc-hci-events": []
  },
  "nfc-manager-tech-discovered": {
    "nfc-manager": []
  },
  "nfc-manager-tech-lost": {
    "nfc-manager": []
  },
  "nfc-manager-send-file": {
    "nfc-manager": []
  },
  "wifip2p-pairing-request": {
    "wifi-manage": []
  },
  "first-run-with-sim": {
    "settings": ["read", "write"]
  }
};


this.SystemMessagePermissionsChecker = {
  











  getSystemMessagePermissions: function getSystemMessagePermissions(aSysMsgName) {
    debug("getSystemMessagePermissions(): aSysMsgName: " + aSysMsgName);

    let permNames = SystemMessagePermissionsTable[aSysMsgName];
    if (permNames === undefined) {
      debug("'" + aSysMsgName + "' is not associated with permissions. " +
            "Please add them to the SystemMessage[Prefix]PermissionsTable.");
      return null;
    }

    let object = { };
    for (let permName in permNames) {
      if (PermissionsTable[permName] === undefined) {
        debug("'" + permName + "' for '" + aSysMsgName + "' is invalid. " +
              "Please correct it in the SystemMessage[Prefix]PermissionsTable.");
        return null;
      }

      
      let access = permNames[permName];
      if (!access || !Array.isArray(access)) {
        debug("'" + permName + "' is not associated with access array. " +
              "Please correct it in the SystemMessage[Prefix]PermissionsTable.");
        return null;
      }
      object[permName] = appendAccessToPermName(permName, access);
    }
    return object
  },

  




  isDataStoreSystemMessage: function(aSysMsgName) {
    return aSysMsgName.indexOf('datastore-update-') === 0;
  },

  


  canDeliverDataStoreSystemMessage: function(aSysMsgName, aManifestURL) {
    let store = aSysMsgName.substr('datastore-update-'.length);

    
    let manifestURLs = dataStoreService.getAppManifestURLsForDataStore(store);
    let enumerate = manifestURLs.enumerate();
    while (enumerate.hasMoreElements()) {
      let manifestURL = enumerate.getNext().QueryInterface(Ci.nsISupportsString);
      if (manifestURL == aManifestURL) {
        return true;
      }
    }

    return false;
  },

  











  isSystemMessagePermittedToRegister:
    function isSystemMessagePermittedToRegister(aSysMsgName,
                                                aManifestURL,
                                                aManifest) {
    debug("isSystemMessagePermittedToRegister(): " +
          "aSysMsgName: " + aSysMsgName + ", " +
          "aManifestURL: " + aManifestURL + ", " +
          "aManifest: " + JSON.stringify(aManifest));

    if (this.isDataStoreSystemMessage(aSysMsgName) &&
        this.canDeliverDataStoreSystemMessage(aSysMsgName, aManifestURL)) {
      return true;
    }

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
      if (aManifest.type == "trusted") {
        appStatus = "trusted";
      }
      break;
    default:
      throw new Error("SystemMessagePermissionsChecker.jsm: " +
                      "Cannot decide the app's status. Install cancelled.");
      break;
    }

    
    
    let newManifest = new ManifestHelper(aManifest, aManifestURL, aManifestURL);

    for (let permName in permNames) {
      
      if (!newManifest.permissions || !newManifest.permissions[permName]) {
        debug("'" + aSysMsgName + "' isn't permitted by '" + permName + "'. " +
              "Please add the permission for app: '" + aManifestURL + "'.");
        return false;
      }
      let permValue = PermissionsTable[permName][appStatus];
      if (permValue != Ci.nsIPermissionManager.PROMPT_ACTION &&
          permValue != Ci.nsIPermissionManager.ALLOW_ACTION) {
        debug("'" + aSysMsgName + "' isn't permitted by '" + permName + "'. " +
              "Please add the permission for app: '" + aManifestURL + "'.");
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
    function isSystemMessagePermittedToSend(aSysMsgName, aPageURL, aManifestURL) {
    debug("isSystemMessagePermittedToSend(): " +
          "aSysMsgName: " + aSysMsgName + ", " +
          "aPageURL: " + aPageURL + ", " +
          "aManifestURL: " + aManifestURL);

    if (this.isDataStoreSystemMessage(aSysMsgName) &&
        this.canDeliverDataStoreSystemMessage(aSysMsgName, aManifestURL)) {
      return true;
    }

    let permNames = this.getSystemMessagePermissions(aSysMsgName);
    if (permNames === null) {
      return false;
    }

    let pageURI = Services.io.newURI(aPageURL, null, null);
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
