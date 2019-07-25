



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const kProtocolName = "b2g-camera:";

let CameraContent = function() {
  this.hasPrivileges = false;
  this.mapping = [];
}
 
CameraContent.prototype = {
  getCameraURI: function(aOptions) {
    if (!this.hasPrivileges)
      return null;

    let options = aOptions || { };
    if (!options.camera)
      options.camera = 0;
    if (!options.width)
      options.width = 320;
    if (!options.height)
      options.height = 240;

    let uuid = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator).generateUUID().toString();
    uuid = uuid.substring(1, uuid.length - 2); 
    this.mapping.push(uuid);
    let uri = kProtocolName + "?camera=" + options.camera + 
                              "&width=" + options.width +
                              "&height=" + options.height +
                              "&type=video/x-raw-yuv";
    
    Services.prefs.setCharPref("b2g.camera." + kProtocolName + "?" + uuid, uri);
    return kProtocolName + "?" + uuid;
  },
  
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "inner-window-destroyed") {
      let wId = aSubject.QueryInterface(Ci.nsISupportsPRUint64).data;
      if (wId == this.innerWindowID) {
        Services.obs.removeObserver(this, "inner-window-destroyed");
        for (let aId in this.mapping)
          Services.prefs.clearUserPref("b2g.camera." + kProtocolName + "?" + aId);
        this.mapping = null;
      }
    }
  },

  init: function(aWindow) {
    let principal = aWindow.document.nodePrincipal;
    let secMan = Cc["@mozilla.org/scriptsecuritymanager;1"].getService(Ci.nsIScriptSecurityManager);

    let perm = principal == secMan.getSystemPrincipal() ? Ci.nsIPermissionManager.ALLOW_ACTION : Services.perms.testExactPermission(principal.URI, "content-camera");

    
    this.hasPrivileges = perm == Ci.nsIPermissionManager.ALLOW_ACTION || from.schemeIs("chrome");

    Services.obs.addObserver(this, "inner-window-destroyed", false);
    let util = aWindow.QueryInterface(Ci.nsIInterfaceRequestor).getInterface(Ci.nsIDOMWindowUtils);
    this.innerWindowID = util.currentInnerWindowID;
  },
 
  classID: Components.ID("{eff4231b-abce-4f7f-a40a-d646e8fde3ce}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIB2GCameraContent, Ci.nsIDOMGlobalPropertyInitializer, Ci.nsIObserver]),
  
  classInfo: XPCOMUtils.generateCI({classID: Components.ID("{eff4231b-abce-4f7f-a40a-d646e8fde3ce}"),
                                    contractID: "@mozilla.org/b2g-camera-content;1",
                                    interfaces: [Ci.nsIB2GCameraContent],
                                    flags: Ci.nsIClassInfo.DOM_OBJECT,
                                    classDescription: "B2G Camera Content Helper"})
}
 
const NSGetFactory = XPCOMUtils.generateNSGetFactory([CameraContent]);
