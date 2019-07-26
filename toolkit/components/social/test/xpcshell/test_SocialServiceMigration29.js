



Cu.import("resource://gre/modules/Services.jsm");


function run_test() {
  
  
  initApp();

  
  
  let manifest = { 
    name: "provider 1",
    origin: "https://example1.com",
  };

  MANIFEST_PREFS.setCharPref(manifest.origin, JSON.stringify(manifest));

  
  let activeVal = Cc["@mozilla.org/supports-string;1"].
             createInstance(Ci.nsISupportsString);
  let active = {};
  active[manifest.origin] = 1;
  activeVal.data = JSON.stringify(active);
  Services.prefs.setComplexValue("social.activeProviders",
                                 Ci.nsISupportsString, activeVal);

  
  
  
  Services.prefs.setBoolPref("social.enabled", false);

  Cu.import("resource://gre/modules/SocialService.jsm");

  let runner = new AsyncRunner();
  let next = runner.next.bind(runner);
  runner.appendIterator(testMigration(manifest, next));
  runner.next();
}

function testMigration(manifest, next) {
  
  
  do_check_true(Services.prefs.prefHasUserValue("social.enabled"));
  do_check_true(MANIFEST_PREFS.prefHasUserValue(manifest.origin));
  
  yield SocialService.getProviderList(next);
  do_check_false(SocialService.enabled);
  do_check_false(Services.prefs.prefHasUserValue("social.enabled"));
  do_check_true(Services.prefs.prefHasUserValue("social.activeProviders"));

  let activeProviders;
  let pref = Services.prefs.getComplexValue("social.activeProviders",
                                            Ci.nsISupportsString).data;
  activeProviders = JSON.parse(pref);
  do_check_true(activeProviders[manifest.origin] == undefined);
  do_check_true(MANIFEST_PREFS.prefHasUserValue(manifest.origin));
}
