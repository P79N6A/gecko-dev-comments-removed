



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function CommandLineHandler() {}

CommandLineHandler.prototype = {
  classID: Components.ID("{6d69c782-40a3-469b-8bfd-3ee366105a4a}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler]),

  handle: function handle(cmdLine) {
    
    Services.ww.openWindow(null,
                           "chrome://webapprt/content/webapp.xul",
                           "_blank",
                           "chrome,dialog=no,resizable,scrollbars",
                           []);

    
    Cu.import("resource://webapprt/modules/WebappsHandler.jsm");
    WebappsHandler.init();
  },

  helpInfo : "",
};

let components = [CommandLineHandler];
let NSGetFactory = XPCOMUtils.generateNSGetFactory(components);










try {
  
  Cu.import("resource://gre/modules/Webapps.jsm");

  
  if (!Services.prefs.getBoolPref("webapprt.firstrun")) {
    Cu.import("resource://webapprt/modules/WebappRT.jsm");
    let uri = Services.io.newURI(WebappRT.config.app.origin, null, null);

    
    Services.perms.add(uri, "pin-app", Ci.nsIPermissionManager.ALLOW_ACTION);
    Services.perms.add(uri, "offline-app",
                       Ci.nsIPermissionManager.ALLOW_ACTION);

    Services.perms.add(uri, "indexedDB", Ci.nsIPermissionManager.ALLOW_ACTION);
    Services.perms.add(uri, "indexedDB-unlimited",
                       Ci.nsIPermissionManager.ALLOW_ACTION);

    
    
    Services.prefs.setBoolPref("webapprt.firstrun", true);
  }
} catch(ex) {
#ifdef MOZ_DEBUG
  dump(ex + "\n");
#endif
}
