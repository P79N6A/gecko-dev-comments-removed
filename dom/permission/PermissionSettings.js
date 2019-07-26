



"use strict";

function debug(aMsg) {
  
}

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PermissionsTable.jsm");

var cpm = Cc["@mozilla.org/childprocessmessagemanager;1"].getService(Ci.nsISyncMessageSender);



const PERMISSIONSETTINGS_CONTRACTID = "@mozilla.org/permissionSettings;1";
const PERMISSIONSETTINGS_CID        = Components.ID("{cd2cf7a1-f4c1-487b-8c1b-1a71c7097431}");
const nsIDOMPermissionSettings      = Ci.nsIDOMPermissionSettings;

function PermissionSettings()
{
  debug("Constructor");
}

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

PermissionSettings.prototype = {
  get: function get(aPermName, aManifestURL, aOrigin, aBrowserFlag) {
    debug("Get called with: " + aPermName + ", " + aManifestURL + ", " + aOrigin + ", " + aBrowserFlag);
    let uri = Services.io.newURI(aOrigin, null, null);
    let appID = appsService.getAppLocalIdByManifestURL(aManifestURL);
    let principal = secMan.getAppCodebasePrincipal(uri, appID, aBrowserFlag);
    let result = permissionManager.testExactPermanentPermission(principal, aPermName);

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

  isExplicit: function isExplicit(aPermName, aManifestURL, aOrigin,
                                  aBrowserFlag) {
    debug("isExplicit: " + aPermName + ", " + aManifestURL + ", " + aOrigin);
    let uri = Services.io.newURI(aOrigin, null, null);
    let appID = appsService.getAppLocalIdByManifestURL(aManifestURL);
    let principal = secMan.getAppCodebasePrincipal(uri, appID, aBrowserFlag);

    return isExplicitInPermissionsTable(aPermName, principal.appStatus);
  },

  set: function set(aPermName, aPermValue, aManifestURL, aOrigin,
                    aBrowserFlag) {
    debug("Set called with: " + aPermName + ", " + aManifestURL + ", " +
          aOrigin + ",  " + aPermValue + ", " + aBrowserFlag);
    let currentPermValue = this.get(aPermName, aManifestURL, aOrigin,
                                    aBrowserFlag);
    let action;
    
    
    if (currentPermValue === "unknown" ||
        aPermValue === "unknown" ||
        !this.isExplicit(aPermName, aManifestURL, aOrigin, aBrowserFlag)) {
      let errorMsg = "PermissionSettings.js: '" + aPermName + "'" +
                     " is an implicit permission for '" + aManifestURL +
                     "' or the permission isn't set";
      Cu.reportError(errorMsg);
      throw new Components.Exception(errorMsg);
    }

    cpm.sendSyncMessage("PermissionSettings:AddPermission", {
      type: aPermName,
      origin: aOrigin,
      manifestURL: aManifestURL,
      value: aPermValue,
      browserFlag: aBrowserFlag
    });
  },

  remove: function remove(aPermName, aManifestURL, aOrigin, aBrowserFlag) {
    let uri = Services.io.newURI(aOrigin, null, null);
    let appID = appsService.getAppLocalIdByManifestURL(aManifestURL);
    let principal = secMan.getAppCodebasePrincipal(uri, appID, aBrowserFlag);

    if (principal.appStatus !== Ci.nsIPrincipal.APP_STATUS_NOT_INSTALLED) {
      let errorMsg = "PermissionSettings.js: '" + aOrigin + "'" +
                     " is installed, cannot remove permission '"+aPermName+"'.";
      Cu.reportError(errorMsg);
      throw new Components.Exception(errorMsg);
    }

    
    cpm.sendSyncMessage("PermissionSettings:AddPermission", {
      type: aPermName,
      origin: aOrigin,
      manifestURL: aManifestURL,
      value: "unknown",
      browserFlag: aBrowserFlag
    });
  },

  init: function init(aWindow) {
    debug("init");

    
    let perm = Services.perms.testExactPermissionFromPrincipal(aWindow.document.nodePrincipal, "permissions");
    if (!Services.prefs.getBoolPref("dom.mozPermissionSettings.enabled")
        || perm != Ci.nsIPermissionManager.ALLOW_ACTION) {
      return null;
    }

    debug("Permission to get/set permissions granted!");
  },

  classID : PERMISSIONSETTINGS_CID,
  QueryInterface : XPCOMUtils.generateQI([nsIDOMPermissionSettings, Ci.nsIDOMGlobalPropertyInitializer]),

  classInfo : XPCOMUtils.generateCI({classID: PERMISSIONSETTINGS_CID,
                                     contractID: PERMISSIONSETTINGS_CONTRACTID,
                                     classDescription: "PermissionSettings",
                                     interfaces: [nsIDOMPermissionSettings],
                                     flags: Ci.nsIClassInfo.DOM_OBJECT})
}

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([PermissionSettings])
