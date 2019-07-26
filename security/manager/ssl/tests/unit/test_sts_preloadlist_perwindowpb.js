var Cc = Components.classes;
var Ci = Components.interfaces;

Components.utils.import("resource://gre/modules/Services.jsm");

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




var hosts = ["http://keyerror.com", "http://subdomain.intercom.io",
             "http://subdomain.pixi.me", "http://crypto.cat",
             "http://logentries.com"];

function cleanup() {
  Services.obs.removeObserver(gObserver, "last-pb-context-exited");

  for (var host of hosts) {
    var uri = Services.io.newURI(host, null, null);
    gSTSService.removeStsState(uri, 0);
  }
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
  do_check_false(gSTSService.isStsHost("factor.cc", 0));
  Services.prefs.setBoolPref("network.stricttransportsecurity.preloadlist", true);
  do_check_true(gSTSService.isStsHost("factor.cc", 0));

  
  do_check_true(gSTSService.isStsHost("arivo.com.br", 0));

  
  do_check_true(gSTSService.isStsHost("subdomain.arivo.com.br", 0));

  
  do_check_true(gSTSService.isStsHost("a.b.c.subdomain.arivo.com.br", 0));

  
  do_check_true(gSTSService.isStsHost("neg9.org", 0));

  
  do_check_false(gSTSService.isStsHost("subdomain.neg9.org", 0));

  
  do_check_true(gSTSService.isStsHost("www.noisebridge.net", 0));

  
  do_check_false(gSTSService.isStsHost("a.subdomain.www.noisebridge.net", 0));

  
  do_check_false(gSTSService.isStsHost("notsts.nonexistent.mozilla.com.", 0));

  
  
  var uri = Services.io.newURI("http://keyerror.com", null, null);
  gSTSService.processStsHeader(uri, "max-age=0", 0);
  do_check_false(gSTSService.isStsHost("keyerror.com", 0));
  do_check_false(gSTSService.isStsHost("subdomain.keyerror.com", 0));
  
  
  gSTSService.processStsHeader(uri, "max-age=1000", 0);
  do_check_true(gSTSService.isStsHost("keyerror.com", 0));
  
  do_check_false(gSTSService.isStsHost("subdomain.keyerror.com", 0));

  
  
  var uri = Services.io.newURI("http://subdomain.intercom.io", null, null);
  gSTSService.processStsHeader(uri, "max-age=0", 0);
  do_check_true(gSTSService.isStsHost("intercom.io", 0));
  do_check_false(gSTSService.isStsHost("subdomain.intercom.io", 0));

  var uri = Services.io.newURI("http://subdomain.pixi.me", null, null);
  gSTSService.processStsHeader(uri, "max-age=0", 0);
  
  
  
  
  
  
  
  
  
  do_check_true(gSTSService.isStsHost("subdomain.pixi.me", 0));
  do_check_true(gSTSService.isStsHost("sibling.pixi.me", 0));
  do_check_true(gSTSService.isStsHost("another.subdomain.pixi.me", 0));

  gSTSService.processStsHeader(uri, "max-age=1000", 0);
  
  
  
  
  
  do_check_true(gSTSService.isStsHost("subdomain.pixi.me", 0));
  do_check_true(gSTSService.isStsHost("sibling.pixi.me", 0));
  do_check_false(gSTSService.isStsHost("another.subdomain.pixi.me", 0));

  
  Services.obs.notifyObservers(null, "last-pb-context-exited", null);
}

const IS_PRIVATE = Ci.nsISocketProvider.NO_PERMANENT_STORAGE;

function test_private_browsing1() {
  
  do_check_true(gSTSService.isStsHost("crypto.cat", IS_PRIVATE));
  do_check_true(gSTSService.isStsHost("a.b.c.subdomain.crypto.cat", IS_PRIVATE));

  var uri = Services.io.newURI("http://crypto.cat", null, null);
  gSTSService.processStsHeader(uri, "max-age=0", IS_PRIVATE);
  do_check_false(gSTSService.isStsHost("crypto.cat", IS_PRIVATE));
  do_check_false(gSTSService.isStsHost("a.b.subdomain.crypto.cat", IS_PRIVATE));

  
  gSTSService.processStsHeader(uri, "max-age=1000", IS_PRIVATE);
  do_check_true(gSTSService.isStsHost("crypto.cat", IS_PRIVATE));
  
  do_check_false(gSTSService.isStsHost("b.subdomain.crypto.cat", IS_PRIVATE));

  
  gSTSService.processStsHeader(uri, "max-age=0", IS_PRIVATE);
  do_check_false(gSTSService.isStsHost("crypto.cat", IS_PRIVATE));
  do_check_false(gSTSService.isStsHost("subdomain.crypto.cat", IS_PRIVATE));

  
  
  
  
  
  
  
  
  
  do_check_true(gSTSService.isStsHost("logentries.com", IS_PRIVATE));
  var uri = Services.io.newURI("http://logentries.com", null, null);
  
  
  gSTSService.processStsHeader(uri, "max-age=-1000", IS_PRIVATE);
  do_check_false(gSTSService.isStsHost("logentries.com", IS_PRIVATE));

  
  Services.obs.notifyObservers(null, "last-pb-context-exited", null);
}

function test_private_browsing2() {
  
  do_check_true(gSTSService.isStsHost("crypto.cat", 0));
  
  do_check_true(gSTSService.isStsHost("subdomain.crypto.cat", 0));

  
  
  do_check_true(gSTSService.isStsHost("logentries.com", 0));

  run_next_test();
}
