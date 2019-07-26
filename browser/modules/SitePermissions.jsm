



this.EXPORTED_SYMBOLS = [ "SitePermissions" ];

Components.utils.import("resource://gre/modules/Services.jsm");

let gStringBundle =
  Services.strings.createBundle("chrome://browser/locale/sitePermissions.properties");

this.SitePermissions = {

  UNKNOWN: Services.perms.UNKNOWN_ACTION,
  ALLOW: Services.perms.ALLOW_ACTION,
  BLOCK: Services.perms.DENY_ACTION,
  SESSION: Components.interfaces.nsICookiePermission.ACCESS_SESSION,

  



  isSupportedURI: function (aURI) {
    return aURI.schemeIs("http") || aURI.schemeIs("https");
  },

  

  listPermissions: function () {
    return Object.keys(gPermissionObject);
  },

  


  getAvailableStates: function (aPermissionID) {
    return gPermissionObject[aPermissionID].states ||
           [ SitePermissions.ALLOW, SitePermissions.BLOCK ];
  },

  

  get: function (aURI, aPermissionID) {
    if (!this.isSupportedURI(aURI))
      return this.UNKNOWN;

    let state;
    if (gPermissionObject[aPermissionID].exactHostMatch)
      state = Services.perms.testExactPermission(aURI, aPermissionID);
    else
      state = Services.perms.testPermission(aURI, aPermissionID);
    return state;
  },

  

  set: function (aURI, aPermissionID, aState) {
    if (!this.isSupportedURI(aURI))
      return;

    Services.perms.add(aURI, aPermissionID, aState);

    if (gPermissionObject[aPermissionID].onSet)
      gPermissionObject[aPermissionID].onSet(aURI, aState);
  },

  


  getPermissionLabel: function (aPermissionID) {
    return gStringBundle.GetStringFromName("permission." + aPermissionID + ".label");
  },

  


  getStateLabel: function (aState) {
    switch (aState) {
      case this.ALLOW:
        return gStringBundle.GetStringFromName("allow");
      case this.SESSION:
        return gStringBundle.GetStringFromName("allowForSession");
      case this.BLOCK:
        return gStringBundle.GetStringFromName("block");
      default:
        throw new Error("unknown permission state");
    }
  }
};

let gPermissionObject = {
  















  "image": {},

  "cookie": {
    states: [ SitePermissions.ALLOW, SitePermissions.SESSION, SitePermissions.BLOCK ]
  },

  "desktop-notification": {},

  "popup": {},

  "install": {},

  "geo": {
    exactHostMatch: true
  },

  "indexedDB": {
    onSet: function (aURI, aState) {
      if (aState == SitePermissions.ALLOW || aState == SitePermissions.BLOCK)
        Services.perms.remove(aURI.host, "indexedDB-unlimited");
    }
  },

  "fullscreen": {},

  "pointerLock": {
    exactHostMatch: true
  }
};
