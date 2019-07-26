



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
    let array = Object.keys(gPermissionObject);
    array.sort((a, b) => {
      return this.getPermissionLabel(a).localeCompare(this.getPermissionLabel(b));
    });
    return array;
  },

  


  getAvailableStates: function (aPermissionID) {
    if (aPermissionID in gPermissionObject &&
        gPermissionObject[aPermissionID].states)
      return gPermissionObject[aPermissionID].states;

    if (this.getDefault(aPermissionID) == this.UNKNOWN)
      return [ SitePermissions.UNKNOWN, SitePermissions.ALLOW, SitePermissions.BLOCK ];

    return [ SitePermissions.ALLOW, SitePermissions.BLOCK ];
  },

  

  getDefault: function (aPermissionID) {
    if (aPermissionID in gPermissionObject &&
        gPermissionObject[aPermissionID].getDefault)
      return gPermissionObject[aPermissionID].getDefault();

    return this.UNKNOWN;
  },

  

  get: function (aURI, aPermissionID) {
    if (!this.isSupportedURI(aURI))
      return this.UNKNOWN;

    let state;
    if (aPermissionID in gPermissionObject &&
        gPermissionObject[aPermissionID].exactHostMatch)
      state = Services.perms.testExactPermission(aURI, aPermissionID);
    else
      state = Services.perms.testPermission(aURI, aPermissionID);
    return state;
  },

  

  set: function (aURI, aPermissionID, aState) {
    if (!this.isSupportedURI(aURI))
      return;

    if (aState == this.UNKNOWN) {
      this.remove(aURI, aPermissionID);
      return;
    }

    Services.perms.add(aURI, aPermissionID, aState);

    if (aPermissionID in gPermissionObject &&
        gPermissionObject[aPermissionID].onChange)
      gPermissionObject[aPermissionID].onChange(aURI, aState);
  },

  

  remove: function (aURI, aPermissionID) {
    if (!this.isSupportedURI(aURI))
      return;

    Services.perms.remove(aURI.host, aPermissionID);

    if (aPermissionID in gPermissionObject &&
        gPermissionObject[aPermissionID].onChange)
      gPermissionObject[aPermissionID].onChange(aURI, this.UNKNOWN);
  },

  


  getPermissionLabel: function (aPermissionID) {
    return gStringBundle.GetStringFromName("permission." + aPermissionID + ".label");
  },

  


  getStateLabel: function (aPermissionID, aState) {
    if (aPermissionID in gPermissionObject &&
        gPermissionObject[aPermissionID].getStateLabel) {
      let label = gPermissionObject[aPermissionID].getStateLabel(aState);
      if (label)
        return label;
    }

    switch (aState) {
      case this.UNKNOWN:
        return gStringBundle.GetStringFromName("alwaysAsk");
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
  

























  "image": {
    getDefault: function () {
      return Services.prefs.getIntPref("permissions.default.image") == 2 ?
               SitePermissions.BLOCK : SitePermissions.ALLOW;
    }
  },

  "cookie": {
    states: [ SitePermissions.ALLOW, SitePermissions.SESSION, SitePermissions.BLOCK ],
    getDefault: function () {
      if (Services.prefs.getIntPref("network.cookie.cookieBehavior") == 2)
        return SitePermissions.BLOCK;

      if (Services.prefs.getIntPref("network.cookie.lifetimePolicy") == 2)
        return SitePermissions.SESSION;

      return SitePermissions.ALLOW;
    }
  },

  "desktop-notification": {},

  "popup": {
    getDefault: function () {
      return Services.prefs.getBoolPref("dom.disable_open_during_load") ?
               SitePermissions.BLOCK : SitePermissions.ALLOW;
    }
  },

  "install": {
    getDefault: function () {
      return Services.prefs.getBoolPref("xpinstall.whitelist.required") ?
               SitePermissions.BLOCK : SitePermissions.ALLOW;
    }
  },

  "geo": {
    exactHostMatch: true
  },

  "indexedDB": {
    states: [ SitePermissions.ALLOW, SitePermissions.UNKNOWN, SitePermissions.BLOCK ],
    getStateLabel: function (aState) {
      
      
      switch (aState) {
        case SitePermissions.UNKNOWN:
          return gStringBundle.GetStringFromName("allow");
        case SitePermissions.ALLOW:
          return gStringBundle.GetStringFromName("alwaysAsk");
        default:
          return null;
      }
    },
    onChange: function (aURI, aState) {
      if (aState == SitePermissions.ALLOW || aState == SitePermissions.BLOCK)
        Services.perms.remove(aURI.host, "indexedDB-unlimited");
    }
  },

  "fullscreen": {},

  "pointerLock": {
    exactHostMatch: true
  }
};
