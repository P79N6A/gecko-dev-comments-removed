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




var hosts = ["http://keyerror.com", "http://subdomain.intercom.io",
             "http://subdomain.pixi.me", "http://crypto.cat",
             "http://logentries.com"];

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

  
  
  
  Services.prefs.setBoolPref("network.stricttransportsecurity.preloadlist", false);
  do_check_false(gSTSService.isStsHost("factor.cc"));
  Services.prefs.setBoolPref("network.stricttransportsecurity.preloadlist", true);
  do_check_true(gSTSService.isStsHost("factor.cc"));

  
  do_check_true(gSTSService.isStsHost("arivo.com.br"));

  
  do_check_true(gSTSService.isStsHost("subdomain.arivo.com.br"));

  
  do_check_true(gSTSService.isStsHost("a.b.c.subdomain.arivo.com.br"));

  
  do_check_true(gSTSService.isStsHost("neg9.org"));

  
  do_check_false(gSTSService.isStsHost("subdomain.neg9.org"));

  
  do_check_true(gSTSService.isStsHost("www.noisebridge.net"));

  
  do_check_false(gSTSService.isStsHost("a.subdomain.www.noisebridge.net"));

  
  do_check_false(gSTSService.isStsHost("notsts.nonexistent.mozilla.com."));

  
  
  var uri = Services.io.newURI("http://keyerror.com", null, null);
  gSTSService.processStsHeader(uri, "max-age=0");
  do_check_false(gSTSService.isStsHost("keyerror.com"));
  do_check_false(gSTSService.isStsHost("subdomain.keyerror.com"));
  
  
  gSTSService.processStsHeader(uri, "max-age=1000");
  do_check_true(gSTSService.isStsHost("keyerror.com"));
  
  do_check_false(gSTSService.isStsHost("subdomain.keyerror.com"));

  
  
  var uri = Services.io.newURI("http://subdomain.intercom.io", null, null);
  gSTSService.processStsHeader(uri, "max-age=0");
  do_check_true(gSTSService.isStsHost("intercom.io"));
  do_check_false(gSTSService.isStsHost("subdomain.intercom.io"));

  var uri = Services.io.newURI("http://subdomain.pixi.me", null, null);
  gSTSService.processStsHeader(uri, "max-age=0");
  
  
  
  
  
  
  
  
  
  do_check_true(gSTSService.isStsHost("subdomain.pixi.me"));
  do_check_true(gSTSService.isStsHost("sibling.pixi.me"));
  do_check_true(gSTSService.isStsHost("another.subdomain.pixi.me"));

  gSTSService.processStsHeader(uri, "max-age=1000");
  
  
  
  
  
  do_check_true(gSTSService.isStsHost("subdomain.pixi.me"));
  do_check_true(gSTSService.isStsHost("sibling.pixi.me"));
  do_check_false(gSTSService.isStsHost("another.subdomain.pixi.me"));

  
  
  
  
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

  
  
  
  
  
  
  
  
  
  do_check_true(gSTSService.isStsHost("logentries.com"));
  var uri = Services.io.newURI("http://logentries.com", null, null);
  
  
  gSTSService.processStsHeader(uri, "max-age=-1000");
  do_check_false(gSTSService.isStsHost("logentries.com"));

  
  getPBSvc().privateBrowsingEnabled = false;
}

function test_private_browsing2() {
  
  do_check_true(gSTSService.isStsHost("crypto.cat"));
  
  do_check_true(gSTSService.isStsHost("subdomain.crypto.cat"));

  
  
  do_check_true(gSTSService.isStsHost("logentries.com"));

  run_next_test();
}
