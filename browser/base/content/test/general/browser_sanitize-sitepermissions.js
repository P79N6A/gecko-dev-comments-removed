

let tempScope = {};
Cc["@mozilla.org/moz/jssubscript-loader;1"].getService(Ci.mozIJSSubScriptLoader)
                                           .loadSubScript("chrome://browser/content/sanitize.js", tempScope);
let Sanitizer = tempScope.Sanitizer;

function countPermissions() {
  let result = 0;
  let enumerator = Services.perms.enumerator;
  while (enumerator.hasMoreElements()) {
    result++;
    enumerator.getNext();
  }
  return result;
}

function test() {
  
  
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

  
  
  let numAtStart = countPermissions();

  
  var pm = Services.perms;
  pm.add(makeURI("http://example.com"), "testing", pm.ALLOW_ACTION);

  
  ok(pm.enumerator.hasMoreElements(), "Permission manager should have elements, since we just added one");

  
  s.sanitize();

  
  is(numAtStart, countPermissions(), "Permission manager should have the same count it started with");
}
