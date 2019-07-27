





var gSSService = Cc["@mozilla.org/ssservice;1"]
                   .getService(Ci.nsISiteSecurityService);

function Observer() {}
Observer.prototype = {
  observe: function(subject, topic, data) {
    if (topic == "last-pb-context-exited")
      run_next_test();
  }
};

var gObserver = new Observer();

function cleanup() {
  Services.obs.removeObserver(gObserver, "last-pb-context-exited");
  gSSService.clearAll();
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
  
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "nonexistent.mozilla.com", 0));

  
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "com", 0));

  
  Services.prefs.setBoolPref("network.stricttransportsecurity.preloadlist", false);
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "bugzilla.mozilla.org", 0));
  Services.prefs.setBoolPref("network.stricttransportsecurity.preloadlist", true);
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "bugzilla.mozilla.org", 0));

  
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "subdomain.bugzilla.mozilla.org", 0));

  
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "a.b.c.def.bugzilla.mozilla.org", 0));

  
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "subdomain.www.torproject.org", 0));

  
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "notsts.nonexistent.mozilla.com.", 0));

  
  
  var uri = Services.io.newURI("http://bugzilla.mozilla.org", null, null);
  gSSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                           "max-age=0", 0);
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "bugzilla.mozilla.org", 0));
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "subdomain.bugzilla.mozilla.org", 0));
  
  
  gSSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                           "max-age=1000", 0);
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "bugzilla.mozilla.org", 0));
  
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "subdomain.bugzilla.mozilla.org", 0));
  gSSService.clearAll();

  
  
  var uri = Services.io.newURI("http://subdomain.www.torproject.org", null, null);
  gSSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                           "max-age=0", 0);
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "www.torproject.org", 0));
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "subdomain.www.torproject.org", 0));

  var uri = Services.io.newURI("http://subdomain.bugzilla.mozilla.org", null, null);
  gSSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                           "max-age=0", 0);
  
  
  
  
  
  
  
  
  
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "bugzilla.mozilla.org", 0));
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "subdomain.bugzilla.mozilla.org", 0));
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "sibling.bugzilla.mozilla.org", 0));
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "another.subdomain.bugzilla.mozilla.org", 0));

  gSSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                           "max-age=1000", 0);
  
  
  
  
  
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "subdomain.bugzilla.mozilla.org", 0));
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "sibling.bugzilla.mozilla.org", 0));
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "another.subdomain.bugzilla.mozilla.org", 0));

  
  
  
  
  
  
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "login.persona.org", 0));
  var uri = Services.io.newURI("http://login.persona.org", null, null);
  gSSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                           "max-age=1", 0);
  do_timeout(1250, function() {
    do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                           "login.persona.org", 0));
    run_next_test();
  });
}

const IS_PRIVATE = Ci.nsISocketProvider.NO_PERMANENT_STORAGE;

function test_private_browsing1() {
  gSSService.clearAll();
  
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "bugzilla.mozilla.org", IS_PRIVATE));
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "a.b.c.subdomain.bugzilla.mozilla.org", IS_PRIVATE));

  var uri = Services.io.newURI("http://bugzilla.mozilla.org", null, null);
  gSSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                           "max-age=0", IS_PRIVATE);
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "bugzilla.mozilla.org", IS_PRIVATE));
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "a.b.subdomain.bugzilla.mozilla.org", IS_PRIVATE));

  
  gSSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                           "max-age=1000", IS_PRIVATE);
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "bugzilla.mozilla.org", IS_PRIVATE));
  
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "b.subdomain.bugzilla.mozilla.org", IS_PRIVATE));

  
  gSSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                           "max-age=0", IS_PRIVATE);
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "bugzilla.mozilla.org", IS_PRIVATE));
  do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                         "subdomain.bugzilla.mozilla.org", IS_PRIVATE));

  
  
  
  
  
  
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "login.persona.org", IS_PRIVATE));
  var uri = Services.io.newURI("http://login.persona.org", null, null);
  gSSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                           "max-age=1", IS_PRIVATE);
  do_timeout(1250, function() {
    do_check_false(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                           "login.persona.org", IS_PRIVATE));
    
    Services.obs.notifyObservers(null, "last-pb-context-exited", null);
  });
}

function test_private_browsing2() {
  
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "bugzilla.mozilla.org", 0));
  
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "subdomain.bugzilla.mozilla.org", 0));

  
  
  do_check_true(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS,
                                        "login.persona.org", 0));

  run_next_test();
}
