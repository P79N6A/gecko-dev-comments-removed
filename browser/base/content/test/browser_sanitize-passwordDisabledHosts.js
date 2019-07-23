


Cc["@mozilla.org/moz/jssubscript-loader;1"].getService(Components.interfaces.mozIJSSubScriptLoader)
                                           .loadSubScript("chrome://browser/content/sanitize.js");

function test() {

  var pwmgr = Components.classes["@mozilla.org/login-manager;1"]
             .getService(Components.interfaces.nsILoginManager);

  
  pwmgr.setLoginSavingEnabled("http://example.com", false);
  
  
  is(pwmgr.getLoginSavingEnabled("http://example.com"), false,
     "example.com should be disabled for password saving since we haven't cleared that yet.");

  
  let s = new Sanitizer();
  s.ignoreTimespan = false;
  s.prefDomain = "privacy.cpd.";
  var itemPrefs = Cc["@mozilla.org/preferences-service;1"]
                  .getService(Components.interfaces.nsIPrefService)
                  .getBranch(s.prefDomain);
  itemPrefs.setBoolPref("history", false);
  itemPrefs.setBoolPref("downloads", false);
  itemPrefs.setBoolPref("cache", false);
  itemPrefs.setBoolPref("cookies", false);
  itemPrefs.setBoolPref("formdata", false);
  itemPrefs.setBoolPref("offlineApps", false);
  itemPrefs.setBoolPref("passwords", false);
  itemPrefs.setBoolPref("sessions", false);
  itemPrefs.setBoolPref("siteSettings", true);
  
  
  s.sanitize();
  
  
  is(pwmgr.getLoginSavingEnabled("http://example.com"), true,
     "example.com should be enabled for password saving again now that we've cleared.");
}
