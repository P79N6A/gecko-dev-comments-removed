



Cu.import("resource://gre/modules/Services.jsm");

const DEFAULT_PREFS = Services.prefs.getDefaultBranch("social.manifest.");

function run_test() {
  
  
  initApp();

  
  
  let manifest = { 
    name: "provider 1",
    origin: "https://example1.com",
  };

  DEFAULT_PREFS.setCharPref(manifest.origin, JSON.stringify(manifest));
  Services.prefs.setBoolPref("social.active", true);

  
  Services.prefs.setBoolPref("social.enabled", true);
  Cu.import("resource://gre/modules/SocialService.jsm");

  let runner = new AsyncRunner();
  let next = runner.next.bind(runner);
  runner.appendIterator(testMigration(manifest, next));
  runner.next();
}

function testMigration(manifest, next) {
  
  
  do_check_false(MANIFEST_PREFS.prefHasUserValue(manifest.origin));
  
  SocialService.getProviderList(function(providers) {
    do_check_true(SocialService.enabled);
    do_check_true(Services.prefs.prefHasUserValue("social.activeProviders"));

    let activeProviders;
    let pref = Services.prefs.getComplexValue("social.activeProviders",
                                              Ci.nsISupportsString);
    activeProviders = JSON.parse(pref);
    do_check_true(activeProviders.has(manifest.origin));
    do_check_true(MANIFEST_PREFS.prefHasUserValue(manifest.origin));
  });
  yield true;
}
