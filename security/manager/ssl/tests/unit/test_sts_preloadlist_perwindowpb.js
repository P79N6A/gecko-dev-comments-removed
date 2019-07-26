





var gSTSService = Cc["@mozilla.org/stsservice;1"]
                  .getService(Ci.nsIStrictTransportSecurityService);

function Observer() {}
Observer.prototype = {
  observe: function(subject, topic, data) {
    if (topic == "last-pb-context-exited")
      run_next_test();
  }
};

var gObserver = new Observer();






function clearStsState() {
  var permissionManager = Cc["@mozilla.org/permissionmanager;1"]
                            .getService(Ci.nsIPermissionManager);
  
  
  var hosts = ["bugzilla.mozilla.org", "login.persona.org",
               "subdomain.www.torproject.org",
               "subdomain.bugzilla.mozilla.org" ];
  for (var host of hosts) {
    permissionManager.remove(host, "sts/use");
    permissionManager.remove(host, "sts/subd");
  }
}

function cleanup() {
  Services.obs.removeObserver(gObserver, "last-pb-context-exited");
  clearStsState();
}

function run_test() {
  do_register_cleanup(cleanup);
  Services.obs.addObserver(gObserver, "last-pb-context-exited", false);

  add_test(test_part1);
  add_test(test_private_browsing1);
  add_test(test_private_browsing2);

  run_next_test();
}

function test_part1() {
  
  do_check_false(gSTSService.isStsHost("nonexistent.mozilla.com", 0));

  
  do_check_false(gSTSService.isStsHost("com", 0));

  
  Services.prefs.setBoolPref("network.stricttransportsecurity.preloadlist", false);
  do_check_false(gSTSService.isStsHost("bugzilla.mozilla.org", 0));
  Services.prefs.setBoolPref("network.stricttransportsecurity.preloadlist", true);
  do_check_true(gSTSService.isStsHost("bugzilla.mozilla.org", 0));

  
  do_check_true(gSTSService.isStsHost("subdomain.bugzilla.mozilla.org", 0));

  
  do_check_true(gSTSService.isStsHost("a.b.c.def.bugzilla.mozilla.org", 0));

  
  do_check_false(gSTSService.isStsHost("subdomain.www.torproject.org", 0));

  
  do_check_false(gSTSService.isStsHost("notsts.nonexistent.mozilla.com.", 0));

  
  
  var uri = Services.io.newURI("http://bugzilla.mozilla.org", null, null);
  gSTSService.processStsHeader(uri, "max-age=0", 0);
  do_check_false(gSTSService.isStsHost("bugzilla.mozilla.org", 0));
  do_check_false(gSTSService.isStsHost("subdomain.bugzilla.mozilla.org", 0));
  
  
  gSTSService.processStsHeader(uri, "max-age=1000", 0);
  do_check_true(gSTSService.isStsHost("bugzilla.mozilla.org", 0));
  
  do_check_false(gSTSService.isStsHost("subdomain.bugzilla.mozilla.org", 0));
  clearStsState();

  
  
  var uri = Services.io.newURI("http://subdomain.www.torproject.org", null, null);
  gSTSService.processStsHeader(uri, "max-age=0", 0);
  do_check_true(gSTSService.isStsHost("www.torproject.org", 0));
  do_check_false(gSTSService.isStsHost("subdomain.www.torproject.org", 0));

  var uri = Services.io.newURI("http://subdomain.bugzilla.mozilla.org", null, null);
  gSTSService.processStsHeader(uri, "max-age=0", 0);
  
  
  
  
  
  
  
  
  
  do_check_true(gSTSService.isStsHost("bugzilla.mozilla.org", 0));
  do_check_true(gSTSService.isStsHost("subdomain.bugzilla.mozilla.org", 0));
  do_check_true(gSTSService.isStsHost("sibling.bugzilla.mozilla.org", 0));
  do_check_true(gSTSService.isStsHost("another.subdomain.bugzilla.mozilla.org", 0));

  gSTSService.processStsHeader(uri, "max-age=1000", 0);
  
  
  
  
  
  do_check_true(gSTSService.isStsHost("subdomain.bugzilla.mozilla.org", 0));
  do_check_true(gSTSService.isStsHost("sibling.bugzilla.mozilla.org", 0));
  do_check_false(gSTSService.isStsHost("another.subdomain.bugzilla.mozilla.org", 0));

  
  Services.obs.notifyObservers(null, "last-pb-context-exited", null);
}

const IS_PRIVATE = Ci.nsISocketProvider.NO_PERMANENT_STORAGE;

function test_private_browsing1() {
  clearStsState();
  
  do_check_true(gSTSService.isStsHost("bugzilla.mozilla.org", IS_PRIVATE));
  do_check_true(gSTSService.isStsHost("a.b.c.subdomain.bugzilla.mozilla.org", IS_PRIVATE));

  var uri = Services.io.newURI("http://bugzilla.mozilla.org", null, null);
  gSTSService.processStsHeader(uri, "max-age=0", IS_PRIVATE);
  do_check_false(gSTSService.isStsHost("bugzilla.mozilla.org", IS_PRIVATE));
  do_check_false(gSTSService.isStsHost("a.b.subdomain.bugzilla.mozilla.org", IS_PRIVATE));

  
  gSTSService.processStsHeader(uri, "max-age=1000", IS_PRIVATE);
  do_check_true(gSTSService.isStsHost("bugzilla.mozilla.org", IS_PRIVATE));
  
  do_check_false(gSTSService.isStsHost("b.subdomain.bugzilla.mozilla.org", IS_PRIVATE));

  
  gSTSService.processStsHeader(uri, "max-age=0", IS_PRIVATE);
  do_check_false(gSTSService.isStsHost("bugzilla.mozilla.org", IS_PRIVATE));
  do_check_false(gSTSService.isStsHost("subdomain.bugzilla.mozilla.org", IS_PRIVATE));

  
  
  
  
  
  
  
  
  
  do_check_true(gSTSService.isStsHost("login.persona.org", IS_PRIVATE));
  var uri = Services.io.newURI("http://login.persona.org", null, null);
  gSTSService.processStsHeader(uri, "max-age=1", IS_PRIVATE);
  do_timeout(1250, function() {
    do_check_false(gSTSService.isStsHost("login.persona.org", IS_PRIVATE));
    
    Services.obs.notifyObservers(null, "last-pb-context-exited", null);
  });
}

function test_private_browsing2() {
  
  do_check_true(gSTSService.isStsHost("bugzilla.mozilla.org", 0));
  
  do_check_true(gSTSService.isStsHost("subdomain.bugzilla.mozilla.org", 0));

  
  
  do_check_true(gSTSService.isStsHost("login.persona.org", 0));

  run_next_test();
}
