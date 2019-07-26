



"use strict"

function debug(str) {
  
}

const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;
const Cc = Components.classes;

const PROMPT_FOR_UNKNOWN = ["audio-capture",
                            "desktop-notification",
                            "geolocation",
                            "video-capture"];


const PERMISSION_NO_SESSION = ["audio-capture", "video-capture"];
const ALLOW_MULTIPLE_REQUESTS = ["audio-capture", "video-capture"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/PermissionsInstaller.jsm");
Cu.import("resource://gre/modules/PermissionsTable.jsm");

var permissionManager = Cc["@mozilla.org/permissionmanager;1"].getService(Ci.nsIPermissionManager);
var secMan = Cc["@mozilla.org/scriptsecuritymanager;1"].getService(Ci.nsIScriptSecurityManager);

let permissionSpecificChecker = {};

XPCOMUtils.defineLazyServiceGetter(this,
                                   "PermSettings",
                                   "@mozilla.org/permissionSettings;1",
                                   "nsIDOMPermissionSettings");

XPCOMUtils.defineLazyServiceGetter(this,
                                   "AudioManager",
                                   "@mozilla.org/telephony/audiomanager;1",
                                   "nsIAudioManager");















function rememberPermission(aTypesInfo, aPrincipal, aSession)
{
  function convertPermToAllow(aPerm, aPrincipal)
  {
    let type =
      permissionManager.testExactPermissionFromPrincipal(aPrincipal, aPerm);
    if (type == Ci.nsIPermissionManager.PROMPT_ACTION ||
        (type == Ci.nsIPermissionManager.UNKNOWN_ACTION &&
        PROMPT_FOR_UNKNOWN.indexOf(aPerm) >= 0)) {
      debug("add " + aPerm + " to permission manager with ALLOW_ACTION");
      if (!aSession) {
        permissionManager.addFromPrincipal(aPrincipal,
                                           aPerm,
                                           Ci.nsIPermissionManager.ALLOW_ACTION);
      } else if (PERMISSION_NO_SESSION.indexOf(aPerm) < 0) {
        permissionManager.addFromPrincipal(aPrincipal,
                                           aPerm,
                                           Ci.nsIPermissionManager.ALLOW_ACTION,
                                           Ci.nsIPermissionManager.EXPIRE_SESSION, 0);
      }
    }
  }

  for (let i in aTypesInfo) {
    
    
    let perm = aTypesInfo[i].permission;
    let access = PermissionsTable[perm].access;
    if (access) {
      for (let idx in access) {
        convertPermToAllow(perm + "-" + access[idx], aPrincipal);
      }
    } else {
      convertPermToAllow(perm, aPrincipal);
    }
  }
}

function ContentPermissionPrompt() {}

ContentPermissionPrompt.prototype = {

  handleExistingPermission: function handleExistingPermission(request,
                                                              typesInfo) {
    typesInfo.forEach(function(type) {
      type.action =
        Services.perms.testExactPermissionFromPrincipal(request.principal,
                                                        type.access);
    });

    
    let checkAllowPermission = function(type) {
      if (type.action == Ci.nsIPermissionManager.ALLOW_ACTION) {
        return true;
      }
      return false;
    }
    if (typesInfo.every(checkAllowPermission)) {
      debug("all permission requests are allowed");
      request.allow();
      return true;
    }

    
    
    let checkDenyPermission = function(type) {
      if (type.action == Ci.nsIPermissionManager.DENY_ACTION ||
          type.action == Ci.nsIPermissionManager.UNKNOWN_ACTION &&
          PROMPT_FOR_UNKNOWN.indexOf(type.access) < 0) {
        return true;
      }
      return false;
    }
    if (typesInfo.every(checkDenyPermission)) {
      debug("all permission requests are denied");
      request.cancel();
      return true;
    }
    return false;
  },

  
  checkMultipleRequest: function checkMultipleRequest(typesInfo) {
    if (typesInfo.length == 1) {
      return true;
    } else if (typesInfo.length > 1) {
      let checkIfAllowMultiRequest = function(type) {
        return (ALLOW_MULTIPLE_REQUESTS.indexOf(type.access) !== -1);
      }
      if (typesInfo.every(checkIfAllowMultiRequest)) {
        debug("legal multiple requests");
        return true;
      }
    }

    return false;
  },

  handledByApp: function handledByApp(request, typesInfo) {
    if (request.principal.appId == Ci.nsIScriptSecurityManager.NO_APP_ID ||
        request.principal.appId == Ci.nsIScriptSecurityManager.UNKNOWN_APP_ID) {
      
      request.cancel();
      return true;
    }

    let appsService = Cc["@mozilla.org/AppsService;1"]
                        .getService(Ci.nsIAppsService);
    let app = appsService.getAppByLocalId(request.principal.appId);

    
    
    let checkIfDenyAppPrincipal = function(type) {
      let url = Services.io.newURI(app.origin, null, null);
      let principal = secMan.getAppCodebasePrincipal(url,
                                                     request.principal.appId,
                                                     false);
      let result = Services.perms.testExactPermissionFromPrincipal(principal,
                                                                   type.access);

      if (result == Ci.nsIPermissionManager.ALLOW_ACTION ||
          result == Ci.nsIPermissionManager.PROMPT_ACTION) {
        type.deny = false;
      }
      return type.deny;
    }
    if (typesInfo.every(checkIfDenyAppPrincipal)) {
      request.cancel();
      return true;
    }

    return false;
  },

  handledByPermissionType: function handledByPermissionType(request, typesInfo) {
    for (let i in typesInfo) {
      if (permissionSpecificChecker.hasOwnProperty(typesInfo[i].permission) &&
          permissionSpecificChecker[typesInfo[i].permission](request)) {
        return true;
      }
    }

    return false;
  },

  _id: 0,
  prompt: function(request) {
    if (secMan.isSystemPrincipal(request.principal)) {
      request.allow();
      return;
    }

    
    let typesInfo = [];
    let perms = request.types.QueryInterface(Ci.nsIArray);
    for (let idx = 0; idx < perms.length; idx++) {
      let perm = perms.queryElementAt(idx, Ci.nsIContentPermissionType);
      let tmp = {
        permission: perm.type,
        access: (perm.access && perm.access !== "unused") ?
                  perm.type + "-" + perm.access : perm.type,
        deny: true,
        action: Ci.nsIPermissionManager.UNKNOWN_ACTION
      };
      typesInfo.push(tmp);
    }
    if (typesInfo.length == 0) {
      request.cancel();
      return;
    }

    if(!this.checkMultipleRequest(typesInfo)) {
      request.cancel();
      return;
    }

    if (this.handledByApp(request, typesInfo) ||
        this.handledByPermissionType(request, typesInfo)) {
      return;
    }

    
    if (this.handleExistingPermission(request, typesInfo)) {
       return;
    }

    
    typesInfo.forEach(function(aType, aIndex) {
      if (aType.action != Ci.nsIPermissionManager.PROMPT_ACTION || aType.deny) {
        typesInfo.splice(aIndex);
      }
    });

    let frame = request.element;
    let requestId = this._id++;

    if (!frame) {
      this.delegatePrompt(request, requestId, typesInfo);
      return;
    }

    frame = frame.wrappedJSObject;
    var cancelRequest = function() {
      frame.removeEventListener("mozbrowservisibilitychange", onVisibilityChange);
      request.cancel();
    }

    var self = this;
    var onVisibilityChange = function(evt) {
      if (evt.detail.visible === true)
        return;

      self.cancelPrompt(request, requestId, typesInfo);
      cancelRequest();
    }

    
    
    let domRequest = frame.getVisible();
    domRequest.onsuccess = function gv_success(evt) {
      if (!evt.target.result) {
        cancelRequest();
        return;
      }

      
      
      frame.addEventListener("mozbrowservisibilitychange", onVisibilityChange);

      self.delegatePrompt(request, requestId, typesInfo, function onCallback() {
        frame.removeEventListener("mozbrowservisibilitychange", onVisibilityChange);
      });
    };

    
    domRequest.onerror = function gv_error() {
      cancelRequest();
    }
  },

  cancelPrompt: function(request, requestId, typesInfo) {
    this.sendToBrowserWindow("cancel-permission-prompt", request, requestId,
                             typesInfo);
  },

  delegatePrompt: function(request, requestId, typesInfo, callback) {

    this.sendToBrowserWindow("permission-prompt", request, requestId, typesInfo,
                             function(type, remember) {
      if (type == "permission-allow") {
        rememberPermission(typesInfo, request.principal, !remember);
        if (callback) {
          callback();
        }
        request.allow();
        return;
      }

      let addDenyPermission = function(type) {
        debug("add " + type.permission +
              " to permission manager with DENY_ACTION");
        if (remember) {
          Services.perms.addFromPrincipal(request.principal, type.access,
                                          Ci.nsIPermissionManager.DENY_ACTION);
        } else {
          Services.perms.addFromPrincipal(request.principal, type.access,
                                          Ci.nsIPermissionManager.DENY_ACTION,
                                          Ci.nsIPermissionManager.EXPIRE_SESSION,
                                          0);
        }
      }
      typesInfo.forEach(addDenyPermission);

      if (callback) {
        callback();
      }
      request.cancel();
    });
  },

  sendToBrowserWindow: function(type, request, requestId, typesInfo, callback) {
    let browser = Services.wm.getMostRecentWindow("navigator:browser");
    let content = browser.getContentWindow();
    if (!content)
      return;

    if (callback) {
      content.addEventListener("mozContentEvent", function contentEvent(evt) {
        let detail = evt.detail;
        if (detail.id != requestId)
          return;
        evt.target.removeEventListener(evt.type, contentEvent);

        callback(detail.type, detail.remember);
      })
    }

    let principal = request.principal;
    let isApp = principal.appStatus != Ci.nsIPrincipal.APP_STATUS_NOT_INSTALLED;
    let remember = (principal.appStatus == Ci.nsIPrincipal.APP_STATUS_PRIVILEGED ||
                    principal.appStatus == Ci.nsIPrincipal.APP_STATUS_CERTIFIED)
                    ? true
                    : request.remember;
    let permissions = [];
    for (let i in typesInfo) {
      debug("prompt " + typesInfo[i].permission);
      permissions.push(typesInfo[i].permission);
    }

    let details = {
      type: type,
      permissions: permissions,
      id: requestId,
      origin: principal.origin,
      isApp: isApp,
      remember: remember
    };

    if (!isApp) {
      browser.shell.sendChromeEvent(details);
      return;
    }

    details.manifestURL = DOMApplicationRegistry.getManifestURLByLocalId(principal.appId);
    browser.shell.sendChromeEvent(details);
  },

  classID: Components.ID("{8c719f03-afe0-4aac-91ff-6c215895d467}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentPermissionPrompt])
};

(function() {
  
  permissionSpecificChecker["audio-capture"] = function(request) {
    if (AudioManager.phoneState === Ci.nsIAudioManager.PHONE_STATE_IN_CALL) {
      request.cancel();
      return true;
    } else {
      return false;
    }
  };
})();


this.NSGetFactory = XPCOMUtils.generateNSGetFactory([ContentPermissionPrompt]);
