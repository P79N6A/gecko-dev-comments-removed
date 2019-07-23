

Cc["@mozilla.org/moz/jssubscript-loader;1"].getService(Ci.mozIJSSubScriptLoader)
                                           .loadSubScript("chrome://browser/content/sanitize.js");

function test() {
  
  
  var pm = Cc["@mozilla.org/permissionmanager;1"]
             .getService(Ci.nsIPermissionManager);
  pm.add(makeURI("http://example.com"), "testing", Ci.nsIPermissionManager.ALLOW_ACTION);
  
  
  ok(pm.enumerator.hasMoreElements(), "Permission manager should have elements, since we just added one");
  
  
  let s = new Sanitizer();
  s.ignoreTimespan = false;
  s.prefDomain = "privacy.cpd.";
  var itemPrefs = gPrefService.getBranch(s.prefDomain);
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
  
  
  ok(!pm.enumerator.hasMoreElements(), "Permission manager shouldn't have entries after Sanitizing");
}
