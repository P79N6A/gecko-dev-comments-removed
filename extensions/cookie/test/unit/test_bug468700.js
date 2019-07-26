


function make_channel(url) {
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var chan = ios.newChannel(url, null, null).QueryInterface(Ci.nsIHttpChannel);
  return chan;
}

function make_uri(url) {
  var ios = Cc["@mozilla.org/network/io-service;1"].
            getService(Ci.nsIIOService);
  return ios.newURI(url, null, null);
}

function run_test() {
  do_load_manifest("cookieprompt.manifest");

  var cs = Cc["@mozilla.org/cookieService;1"].getService(Ci.nsICookieService);
  var cm = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager2);
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);

  var spec = "http://foo.bar/baz";
  var uri = ios.newURI(spec, null, null);

  
  prefs.setIntPref("network.cookie.lifetimePolicy", 0);
  
  cs.setCookieString(uri, null, "foo=bar", null);
  do_check_eq(cs.countCookiesFromHost("foo.bar"), 1);
  
  prefs.setIntPref("network.cookie.lifetimePolicy", 1);
  
  cs.setCookieString(uri, null, "bar=baz", null);
  do_check_eq(cs.countCookiesFromHost("foo.bar"), 1);
  cs.removeAll();


  
  var chan = make_channel(uri.spec);
  chan.QueryInterface(Ci.nsIPrivateBrowsingChannel);
  chan.setPrivate(true);
  
  
  prefs.setIntPref("network.cookie.lifetimePolicy", 0);

  
  cs.setCookieString(uri, null, "foobar=bar", chan);
  do_check_eq(cs.getCookieString(uri, chan), "foobar=bar");
  
  prefs.setIntPref("network.cookie.lifetimePolicy", 1);
  
  cs.setCookieString(uri, null, "foobaz=bar", chan);
  do_check_eq(cs.getCookieString(uri, chan), "foobar=bar; foobaz=bar");
}

