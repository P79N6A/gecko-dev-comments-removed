









const EXPORTED_SYMBOLS = [];

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");


Cu.import("resource://gre/modules/Webapps.jsm");


Cu.import("resource://webapprt/modules/WebappsHandler.jsm");
WebappsHandler.init();


if (!Services.prefs.getBoolPref("webapprt.firstrun")) {
  Cu.import("resource://webapprt/modules/WebappRT.jsm");
  let uri = Services.io.newURI(WebappRT.config.app.origin, null, null);

  
  Services.perms.add(uri, "pin-app",
                     Ci.nsIPermissionManager.ALLOW_ACTION);
  Services.perms.add(uri, "offline-app",
                     Ci.nsIPermissionManager.ALLOW_ACTION);

  Services.perms.add(uri, "indexedDB",
                     Ci.nsIPermissionManager.ALLOW_ACTION);
  Services.perms.add(uri, "indexedDB-unlimited",
                     Ci.nsIPermissionManager.ALLOW_ACTION);

  
  
  Services.prefs.setBoolPref("webapprt.firstrun", true);
}
