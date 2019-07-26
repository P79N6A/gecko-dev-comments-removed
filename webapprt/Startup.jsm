









this.EXPORTED_SYMBOLS = [];

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");


Cu.import("resource://gre/modules/Webapps.jsm");


Cu.import("resource://webapprt/modules/WebappsHandler.jsm");
WebappsHandler.init();


if (!Services.prefs.getBoolPref("webapprt.firstrun")) {
  

  
  
  Services.prefs.setBoolPref("webapprt.firstrun", true);
}
