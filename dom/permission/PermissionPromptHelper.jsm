


















"use strict";

let DEBUG = 0;
let debug;
if (DEBUG)
  debug = function (s) { dump("-*- Permission Prompt Helper component: " + s + "\n"); }
else
  debug = function (s) {}

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

this.EXPORTED_SYMBOLS = ["PermissionPromptHelper"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/PermissionsInstaller.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageListenerManager");

XPCOMUtils.defineLazyServiceGetter(this, "permissionPromptService",
                                   "@mozilla.org/permission-prompt-service;1",
                                   "nsIPermissionPromptService");

var permissionManager = Cc["@mozilla.org/permissionmanager;1"].getService(Ci.nsIPermissionManager);
var secMan = Cc["@mozilla.org/scriptsecuritymanager;1"].getService(Ci.nsIScriptSecurityManager);
var appsService = Cc["@mozilla.org/AppsService;1"].getService(Ci.nsIAppsService);

this.PermissionPromptHelper = {
  init: function() {
    debug("Init");
    ppmm.addMessageListener("PermissionPromptHelper:AskPermission", this);
    Services.obs.addObserver(this, "profile-before-change", false);
  },

  askPermission: function(aMessage, aCallbacks) {
    let msg = aMessage.json;

    let access;
    if (PermissionsTable[msg.type].access) {
      access = "readwrite"; 
    }
    
    var expandedPerms = expandPermissions(msg.type, access);
    let installedPerms = [];
    let principal;

    for (let idx in expandedPerms) {
      let uri = Services.io.newURI(msg.origin, null, null);
      principal =
        secMan.getAppCodebasePrincipal(uri, msg.appID, msg.browserFlag);
      let perm =
        permissionManager.testExactPermissionFromPrincipal(principal, msg.type);
      installedPerms.push(perm);
    }

    
    
    for (let idx in installedPerms) {
      
      if (installedPerms[idx] == Ci.nsIPermissionManager.DENY_ACTION ||
          installedPerms[idx] == Ci.nsIPermissionManager.UNKNOWN_ACTION) {
        aCallbacks.cancel();
        return;
      }
    }

    for (let idx in installedPerms) {
      if (installedPerms[idx] == Ci.nsIPermissionManager.PROMPT_ACTION) {
        
        let request = {
          type: msg.type,
          principal: principal,
          QueryInterface: XPCOMUtils.generateQI([Ci.nsIContentPermissionRequest]),
          allow: aCallbacks.allow,
          cancel: aCallbacks.cancel,
          window: Services.wm.getMostRecentWindow("navigator:browser")
        };

        permissionPromptService.getPermission(request);
        return;
      }
    }

    for (let idx in installedPerms) {
      if (installedPerms[idx] == Ci.nsIPermissionManager.ALLOW_ACTION) {
        aCallbacks.allow();
        return;
      }
    }
  },

  observe: function(aSubject, aTopic, aData) {
    ppmm.removeMessageListener("PermissionPromptHelper:AskPermission", this);
    Services.obs.removeObserver(this, "profile-before-change");
    ppmm = null;
  },

  receiveMessage: function(aMessage) {
    debug("PermissionPromptHelper::receiveMessage " + aMessage.name);
    let mm = aMessage.target;
    let msg = aMessage.data;

    let result;
    if (aMessage.name == "PermissionPromptHelper:AskPermission") {
      this.askPermission(aMessage, {
        cancel: function() {
          mm.sendAsyncMessage("PermissionPromptHelper:AskPermission:OK", {result: Ci.nsIPermissionManager.DENY_ACTION, requestID: msg.requestID});
        },
        allow: function() {
          mm.sendAsyncMessage("PermissionPromptHelper:AskPermission:OK", {result: Ci.nsIPermissionManager.ALLOW_ACTION, requestID: msg.requestID});
        }
      });
    }
  }
}

PermissionPromptHelper.init();
