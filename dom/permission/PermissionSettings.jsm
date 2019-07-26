



"use strict";

function debug(s) {
  
}

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

this.EXPORTED_SYMBOLS = ["PermissionSettingsModule"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PermissionsTable.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyServiceGetter(this,
                                   "permissionManager",
                                   "@mozilla.org/permissionmanager;1",
                                   "nsIPermissionManager");

XPCOMUtils.defineLazyServiceGetter(this,
                                   "secMan",
                                   "@mozilla.org/scriptsecuritymanager;1",
                                   "nsIScriptSecurityManager");

XPCOMUtils.defineLazyServiceGetter(this,
                                   "appsService",
                                   "@mozilla.org/AppsService;1",
                                   "nsIAppsService");

this.PermissionSettingsModule = {
  init: function init() {
    debug("Init");
    ppmm.addMessageListener("PermissionSettings:AddPermission", this);
    Services.obs.addObserver(this, "profile-before-change", false);
  },


  _isChangeAllowed: function(aPrincipal, aPermName, aAction) {
    
    
    
    
    
    
    
    
    
    
    
    
    let perm =
      permissionManager.testExactPermissionFromPrincipal(aPrincipal,aPermName);
    let isExplicit = isExplicitInPermissionsTable(aPermName, aPrincipal.appStatus);

    let deleteAllowed = true;
    if (aAction === "unknown")
      deleteAllowed = (aPrincipal.appStatus === Ci.nsIPrincipal.APP_STATUS_NOT_INSTALLED);

    return deleteAllowed &&
           (perm !== Ci.nsIPermissionManager.UNKNOWN_ACTION) &&
           isExplicit;
  },

  addPermission: function addPermission(aData, aCallbacks) {

    this._internalAddPermission(aData, true, aCallbacks);

  },


  _internalAddPermission: function _internalAddPermission(aData, aAllowAllChanges, aCallbacks) {
    let uri = Services.io.newURI(aData.origin, null, null);
    let appID = appsService.getAppLocalIdByManifestURL(aData.manifestURL);
    let principal = secMan.getAppCodebasePrincipal(uri, appID, aData.browserFlag);

    let action;
    switch (aData.value)
    {
      case "unknown":
        action = Ci.nsIPermissionManager.UNKNOWN_ACTION;
        break;
      case "allow":
        action = Ci.nsIPermissionManager.ALLOW_ACTION;
        break;
      case "deny":
        action = Ci.nsIPermissionManager.DENY_ACTION;
        break;
      case "prompt":
        action = Ci.nsIPermissionManager.PROMPT_ACTION;
        break;
      default:
        dump("Unsupported PermisionSettings Action: " + aData.value +"\n");
        action = Ci.nsIPermissionManager.UNKNOWN_ACTION;
    }

    if (aAllowAllChanges ||
        this._isChangeAllowed(principal, aData.type, aData.value)) {
      debug("add: " + aData.origin + " " + appID + " " + action);
      permissionManager.addFromPrincipal(principal, aData.type, action);
      return true;
    } else {
      debug("add Failure: " + aData.origin + " " + appID + " " + action);
      return false; 
    }
  },

  getPermission: function getPermission(aPermName, aManifestURL, aOrigin, aBrowserFlag) {
    debug("getPermission: " + aPermName + ", " + aManifestURL + ", " + aOrigin);
    let uri = Services.io.newURI(aOrigin, null, null);
    let appID = appsService.getAppLocalIdByManifestURL(aManifestURL);
    let principal = secMan.getAppCodebasePrincipal(uri, appID, aBrowserFlag);
    let result = permissionManager.testExactPermissionFromPrincipal(principal, aPermName);

    switch (result)
    {
      case Ci.nsIPermissionManager.UNKNOWN_ACTION:
        return "unknown";
      case Ci.nsIPermissionManager.ALLOW_ACTION:
        return "allow";
      case Ci.nsIPermissionManager.DENY_ACTION:
        return "deny";
      case Ci.nsIPermissionManager.PROMPT_ACTION:
        return "prompt";
      default:
        dump("Unsupported PermissionSettings Action!\n");
        return "unknown";
    }
  },

  removePermission: function removePermission(aPermName, aManifestURL, aOrigin, aBrowserFlag) {
    let data = {
      type: aPermName,
      origin: aOrigin,
      manifestURL: aManifestURL,
      value: "unknown",
      browserFlag: aBrowserFlag
    };
    this._internalAddPermission(data, false);
  },

  observe: function observe(aSubject, aTopic, aData) {
    ppmm.removeMessageListener("PermissionSettings:AddPermission", this);
    Services.obs.removeObserver(this, "profile-before-change");
    ppmm = null;
  },

  receiveMessage: function receiveMessage(aMessage) {
    debug("PermissionSettings::receiveMessage " + aMessage.name);
    let mm = aMessage.target;
    let msg = aMessage.data;

    let result;
    switch (aMessage.name) {
      case "PermissionSettings:AddPermission":
        let success = false;
        let errorMsg =
              " from a content process with no 'permissions' privileges.";
        if (mm.assertPermission("permissions")) {
          success = this._internalAddPermission(msg, false);
          if (!success) {
            
            mm.assertPermission("permissions-modify-implicit");
            errorMsg = " had an implicit permission change. Child process killed.";
          }
        }

        if (!success) {
          Cu.reportError("PermissionSettings message " + msg.type + errorMsg);
          return null;
        }
        break;
    }
  }
}

PermissionSettingsModule.init();
