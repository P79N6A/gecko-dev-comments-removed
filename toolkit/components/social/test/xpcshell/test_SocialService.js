



Cu.import("resource://gre/modules/Services.jsm");

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "1.9");
  
  var blocklistFile = gProfD.clone();
  blocklistFile.append("blocklist.xml");
  if (blocklistFile.exists())
    blocklistFile.remove(false);
  var source = do_get_file("blocklist.xml");
  source.copyTo(gProfD, "blocklist.xml");

  
  
  let manifests = [
    { 
      name: "provider 1",
      origin: "https://example1.com",
    },
    { 
      name: "provider 2",
      origin: "https://example2.com"
    }
  ];

  manifests.forEach(function (manifest) {
    MANIFEST_PREFS.setCharPref(manifest.origin, JSON.stringify(manifest));
  });
  
  let activeVal = Cc["@mozilla.org/supports-string;1"].
             createInstance(Ci.nsISupportsString);
  let active = {};
  for (let m of manifests)
    active[m.origin] = 1;
  activeVal.data = JSON.stringify(active);
  Services.prefs.setComplexValue("social.activeProviders",
                                 Ci.nsISupportsString, activeVal);
  Services.prefs.setCharPref("social.provider.current", manifests[0].origin);

  
  Services.prefs.setBoolPref("social.enabled", true);
  Cu.import("resource://gre/modules/SocialService.jsm");

  let runner = new AsyncRunner();
  let next = runner.next.bind(runner);
  runner.appendIterator(testGetProvider(manifests, next));
  runner.appendIterator(testGetProviderList(manifests, next));
  runner.appendIterator(testEnabled(manifests, next));
  runner.appendIterator(testAddRemoveProvider(manifests, next));
  runner.appendIterator(testIsSameOrigin(manifests, next));
  runner.appendIterator(testResolveUri  (manifests, next));
  runner.next();
}

function testGetProvider(manifests, next) {
  for (let i = 0; i < manifests.length; i++) {
    let manifest = manifests[i];
    let provider = yield SocialService.getProvider(manifest.origin, next);
    do_check_neq(provider, null);
    do_check_eq(provider.name, manifest.name);
    do_check_eq(provider.workerURL, manifest.workerURL);
    do_check_eq(provider.origin, manifest.origin);
  }
  do_check_eq((yield SocialService.getProvider("bogus", next)), null);
}

function testGetProviderList(manifests, next) {
  let providers = yield SocialService.getProviderList(next);
  do_check_true(providers.length >= manifests.length);
  for (let i = 0; i < manifests.length; i++) {
    let providerIdx = providers.map(function (p) p.origin).indexOf(manifests[i].origin);
    let provider = providers[providerIdx];
    do_check_true(!!provider);
    do_check_false(provider.enabled);
    do_check_eq(provider.workerURL, manifests[i].workerURL);
    do_check_eq(provider.name, manifests[i].name);
  }
}

function testEnabled(manifests, next) {
  
  let providers = yield SocialService.getProviderList(next);
  do_check_true(providers.length >= manifests.length);
  do_check_true(SocialService.enabled);
  providers.forEach(function (provider) {
    do_check_false(provider.enabled);
  });

  let notificationDisabledCorrect = false;
  Services.obs.addObserver(function obs1(subj, topic, data) {
    Services.obs.removeObserver(obs1, "social:pref-changed");
    notificationDisabledCorrect = data == "disabled";
  }, "social:pref-changed", false);

  
  providers[providers.length-1].enabled = true;

  
  SocialService.enabled = false;
  do_check_true(notificationDisabledCorrect);
  do_check_true(!SocialService.enabled);
  providers.forEach(function (provider) {
    do_check_false(provider.enabled);
  });

  SocialService.enabled = true;
  do_check_true(SocialService.enabled);
  
  providers.forEach(function (provider) {
    do_check_false(provider.enabled);
  });
}

function testAddRemoveProvider(manifests, next) {
  var threw;
  try {
    
    SocialService.addProvider(manifests[0]);
  } catch (ex) {
    threw = ex;
  }
  do_check_neq(threw.toString().indexOf("SocialService.addProvider: provider with this origin already exists"), -1);

  let originalProviders = yield SocialService.getProviderList(next);

  
  let newProvider = yield SocialService.addProvider({
    name: "foo",
    origin: "http://example3.com"
  }, next);
  let retrievedNewProvider = yield SocialService.getProvider(newProvider.origin, next);
  do_check_eq(newProvider, retrievedNewProvider);

  let providersAfter = yield SocialService.getProviderList(next);
  do_check_eq(providersAfter.length, originalProviders.length + 1);
  do_check_neq(providersAfter.indexOf(newProvider), -1);

  
  yield SocialService.removeProvider(newProvider.origin, next);
  providersAfter = yield SocialService.getProviderList(next);
  do_check_eq(providersAfter.length, originalProviders.length);
  do_check_eq(providersAfter.indexOf(newProvider), -1);
  newProvider = yield SocialService.getProvider(newProvider.origin, next);
  do_check_true(!newProvider);
}

function testIsSameOrigin(manifests, next) {
  let providers = yield SocialService.getProviderList(next);
  let provider = providers[0];
  
  do_check_true(provider.isSameOrigin(provider.origin));
  do_check_true(provider.isSameOrigin(Services.io.newURI(provider.origin, null, null)));
  do_check_true(provider.isSameOrigin(provider.origin + "/some-sub-page"));
  do_check_true(provider.isSameOrigin(Services.io.newURI(provider.origin + "/some-sub-page", null, null)));
  do_check_false(provider.isSameOrigin("http://something.com"));
  do_check_false(provider.isSameOrigin(Services.io.newURI("http://something.com", null, null)));
  do_check_false(provider.isSameOrigin("data:text/html,<p>hi"));
  do_check_true(provider.isSameOrigin("data:text/html,<p>hi", true));
  do_check_false(provider.isSameOrigin(Services.io.newURI("data:text/html,<p>hi", null, null)));
  do_check_true(provider.isSameOrigin(Services.io.newURI("data:text/html,<p>hi", null, null), true));
  
  do_check_false(provider.isSameOrigin(null));
}

function testResolveUri(manifests, next) {
  let providers = yield SocialService.getProviderList(next);
  let provider = providers[0];
  do_check_eq(provider.resolveUri(provider.origin).spec, provider.origin + "/");
  do_check_eq(provider.resolveUri("foo.html").spec, provider.origin + "/foo.html");
  do_check_eq(provider.resolveUri("/foo.html").spec, provider.origin + "/foo.html");
  do_check_eq(provider.resolveUri("http://somewhereelse.com/foo.html").spec, "http://somewhereelse.com/foo.html");
  do_check_eq(provider.resolveUri("data:text/html,<p>hi").spec, "data:text/html,<p>hi");
}
