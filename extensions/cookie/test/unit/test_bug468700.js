const Cc = Components.classes;
const Ci = Components.interfaces;

function run_test() {
  do_load_module("cookieprompt.js");

  var cs = Cc["@mozilla.org/cookieService;1"].getService(Ci.nsICookieService);
  var cm = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager2);
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);
  var pb = null;
  try {
    pb = Cc["@mozilla.org/privatebrowsing;1"].getService(Ci.nsIPrivateBrowsingService);
  } catch (e) {}

  var spec = "http://foo.bar/baz";
  var uri = ios.newURI(spec, null, null);

  
  prefs.setIntPref("network.cookie.lifetimePolicy", 0);
  
  cs.setCookieString(uri, null, "foo=bar", null);
  do_check_eq(cs.countCookiesFromHost("foo.bar"), 1);
  
  prefs.setIntPref("network.cookie.lifetimePolicy", 1);
  
  cs.setCookieString(uri, null, "bar=baz", null);
  do_check_eq(cs.countCookiesFromHost("foo.bar"), 1);
  cs.removeAll();

  
  if (pb) {
    
    pb.privateBrowsingEnabled = true;

    
    prefs.setIntPref("network.cookie.lifetimePolicy", 0);
    
    cs.setCookieString(uri, null, "foobar=bar", null);
    do_check_eq(cs.countCookiesFromHost("foo.bar"), 1);
    
    prefs.setIntPref("network.cookie.lifetimePolicy", 1);
    
    cs.setCookieString(uri, null, "foobaz=bar", null);
    do_check_eq(cs.countCookiesFromHost("foo.bar"), 2);
  }
}

