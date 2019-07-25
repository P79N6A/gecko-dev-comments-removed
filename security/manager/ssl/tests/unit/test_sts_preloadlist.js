var Cc = Components.classes;
var Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/Services.jsm");

var _pbsvc;



function getPBSvc() {
  if (_pbsvc)
    return _pbsvc;

  try {
    _pbsvc = Cc["@mozilla.org/privatebrowsing;1"]
               .getService(Ci.nsIPrivateBrowsingService);
    return _pbsvc;
  } catch (e) {}

  return null;
}

var gSTSService = Cc["@mozilla.org/stsservice;1"]
                  .getService(Ci.nsIStrictTransportSecurityService);

function Observer() {}
Observer.prototype = {
  observe: function(subject, topic, data) {
    run_next_test();
  }
};

var gObserver = new Observer();




var hosts = ["http://keyerror.com", "http://subdomain.kyps.net",
             "http://subdomain.cert.se", "http://crypto.cat",
             "http://www.logentries.com"];

function cleanup() {
  Services.obs.removeObserver(gObserver, "private-browsing-transition-complete");
  if (getPBSvc())
    getPBSvc().privateBrowsingEnabled = false;

  for (var host of hosts) {
    var uri = Services.io.newURI(host, null, null);
    gSTSService.removeStsState(uri);
  }
}

function run_test() {
  do_register_cleanup(cleanup);
  Services.obs.addObserver(gObserver, "private-browsing-transition-complete", false);
  Services.prefs.setBoolPref("browser.privatebrowsing.keep_current_session", true);

  add_test(test_part1);
  if (getPBSvc()) {
    add_test(test_private_browsing1);
    add_test(test_private_browsing2);
  }

  run_next_test();
}

function test_part1() {
  
  do_check_false(gSTSService.isStsHost("nonexistent.mozilla.com"));

  
  do_check_false(gSTSService.isStsHost("com"));

  
  
  
  do_check_true(gSTSService.isStsHost("health.google.com"));

  
  do_check_true(gSTSService.isStsHost("subdomain.health.google.com"));

  
  do_check_true(gSTSService.isStsHost("a.b.c.subdomain.health.google.com"));

  
  do_check_true(gSTSService.isStsHost("epoxate.com"));

  
  do_check_false(gSTSService.isStsHost("subdomain.epoxate.com"));

  
  do_check_true(gSTSService.isStsHost("www.googlemail.com"));

  
  do_check_false(gSTSService.isStsHost("a.subdomain.www.googlemail.com"));

  
  do_check_false(gSTSService.isStsHost("notsts.nonexistent.mozilla.com."));

  
  
  var uri = Services.io.newURI("http://keyerror.com", null, null);
  gSTSService.processStsHeader(uri, "max-age=0");
  do_check_false(gSTSService.isStsHost("keyerror.com"));
  do_check_false(gSTSService.isStsHost("subdomain.keyerror.com"));
  
  
  gSTSService.processStsHeader(uri, "max-age=1000");
  do_check_true(gSTSService.isStsHost("keyerror.com"));
  
  do_check_false(gSTSService.isStsHost("subdomain.keyerror.com"));

  
  
  var uri = Services.io.newURI("http://subdomain.kyps.net", null, null);
  gSTSService.processStsHeader(uri, "max-age=0");
  do_check_true(gSTSService.isStsHost("kyps.net"));
  do_check_false(gSTSService.isStsHost("subdomain.kyps.net"));

  var uri = Services.io.newURI("http://subdomain.cert.se", null, null);
  gSTSService.processStsHeader(uri, "max-age=0");
  
  
  
  
  
  
  
  
  
  do_check_true(gSTSService.isStsHost("subdomain.cert.se"));
  do_check_true(gSTSService.isStsHost("sibling.cert.se"));
  do_check_true(gSTSService.isStsHost("another.subdomain.cert.se"));

  gSTSService.processStsHeader(uri, "max-age=1000");
  
  
  
  
  
  do_check_true(gSTSService.isStsHost("subdomain.cert.se"));
  do_check_true(gSTSService.isStsHost("sibling.cert.se"));
  do_check_false(gSTSService.isStsHost("another.subdomain.cert.se"));

  
  
  
  
  if (getPBSvc()) {
    getPBSvc().privateBrowsingEnabled = true;
  } else {
    run_next_test();
  }
}

function test_private_browsing1() {
  
  do_check_true(gSTSService.isStsHost("crypto.cat"));
  do_check_true(gSTSService.isStsHost("a.b.c.subdomain.crypto.cat"));

  var uri = Services.io.newURI("http://crypto.cat", null, null);
  gSTSService.processStsHeader(uri, "max-age=0");
  do_check_false(gSTSService.isStsHost("crypto.cat"));
  do_check_false(gSTSService.isStsHost("a.b.subdomain.crypto.cat"));

  
  gSTSService.processStsHeader(uri, "max-age=1000");
  do_check_true(gSTSService.isStsHost("crypto.cat"));
  
  do_check_false(gSTSService.isStsHost("b.subdomain.crypto.cat"));

  
  gSTSService.processStsHeader(uri, "max-age=0");
  do_check_false(gSTSService.isStsHost("crypto.cat"));
  do_check_false(gSTSService.isStsHost("subdomain.crypto.cat"));

  
  
  
  
  
  
  
  
  
  do_check_true(gSTSService.isStsHost("www.logentries.com"));
  var uri = Services.io.newURI("http://www.logentries.com", null, null);
  
  
  gSTSService.processStsHeader(uri, "max-age=-1000");
  do_check_false(gSTSService.isStsHost("www.logentries.com"));

  
  getPBSvc().privateBrowsingEnabled = false;
}

function test_private_browsing2() {
  
  do_check_true(gSTSService.isStsHost("crypto.cat"));
  
  do_check_true(gSTSService.isStsHost("subdomain.crypto.cat"));

  
  
  do_check_true(gSTSService.isStsHost("www.logentries.com"));

  run_next_test();
}
