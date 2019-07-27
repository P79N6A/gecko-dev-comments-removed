


const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

"use strict";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

(function initMobileIdTestingInfrastructure() {
  do_get_profile();

  const PREF_FORCE_HTTPS = "services.mobileid.forcehttps";
  Services.prefs.setBoolPref(PREF_FORCE_HTTPS, false);
  Services.prefs.setCharPref("services.mobileid.loglevel", "Debug");
  Services.prefs.setCharPref("services.mobileid.server.uri",
                             "https://dummyurl.com");
}).call(this);
