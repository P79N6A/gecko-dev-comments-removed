



"use strict";

let DEBUG = 0;
if (DEBUG)
  debug = function (s) { dump("-*- PermissionSettings: " + s + "\n"); }
else
  debug = function (s) {}

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

var cpm = Cc["@mozilla.org/childprocessmessagemanager;1"].getService(Ci.nsISyncMessageSender);



const PERMISSIONSETTINGS_CONTRACTID = "@mozilla.org/permissionSettings;1";
const PERMISSIONSETTINGS_CID        = Components.ID("{18390770-02ab-11e2-a21f-0800200c9a66}");
const nsIDOMPermissionSettings      = Ci.nsIDOMPermissionSettings;

function PermissionSettings()
{
  debug("Constructor");
}

var permissionManager = Cc["@mozilla.org/permissionmanager;1"].getService(Ci.nsIPermissionManager);
var secMan = Cc["@mozilla.org/scriptsecuritymanager;1"].getService(Ci.nsIScriptSecurityManager);
var appsService = Cc["@mozilla.org/AppsService;1"].getService(Ci.nsIAppsService);

PermissionSettings.prototype = {
  get: function get(aPermission, aManifestURL, aOrigin, aBrowserFlag) {
    debug("Get called with: " + aPermission + ", " + aManifestURL + ", " + aOrigin + ", " + aBrowserFlag);
    let uri = Services.io.newURI(aOrigin, null, null);
    let appID = appsService.getAppLocalIdByManifestURL(aManifestURL);
    let principal = secMan.getAppCodebasePrincipal(uri, appID, aBrowserFlag);
    let result = permissionManager.testExactPermissionFromPrincipal(principal, aPermission);

    switch (result)
    {
      case Ci.nsIPermissionManager.UNKNOWN_ACTION:
        return "unknown"
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

  set: function set(aPermission, aValue, aManifestURL, aOrigin, aBrowserFlag) {
    debug("Set called with: " + aPermission + ", " + aManifestURL + ", " + aOrigin + ",  " + aValue + ", " + aBrowserFlag);
    let action;
    cpm.sendSyncMessage("PermissionSettings:AddPermission", {
      type: aPermission,
      origin: aOrigin,
      manifestURL: aManifestURL,
      value: aValue,
      browserFlag: aBrowserFlag
    });
  },

  init: function(aWindow) {
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

const NSGetFactory = XPCOMUtils.generateNSGetFactory([PermissionSettings])
