



const Cc = Components.classes;
const Ci = Components.interfaces;

function run_test() {
  
  let profile = do_get_profile();

  var cs = Cc["@mozilla.org/cookieService;1"].getService(Ci.nsICookieService);
  var cso = cs.QueryInterface(Ci.nsIObserver);
  var cm = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager2);
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var prefs = Cc["@mozilla.org/preferences-service;1"].getService(Ci.nsIPrefBranch);

  
  
  var spec1 = "http://foo.com/foo.html";
  var spec2 = "http://bar.com/bar.html";
  var uri1 = ios.newURI(spec1, null, null);
  var uri2 = ios.newURI(spec2, null, null);
  var channel1 = ios.newChannelFromURI(uri1);
  var channel2 = ios.newChannelFromURI(uri2);

  
  
  var httpchannel1 = channel1.QueryInterface(Ci.nsIHttpChannelInternal);
  var httpchannel2 = channel1.QueryInterface(Ci.nsIHttpChannelInternal);
  httpchannel1.forceAllowThirdPartyCookie = true;
  httpchannel2.forceAllowThirdPartyCookie = true;

  
  prefs.setIntPref("network.cookie.cookieBehavior", 0);
  prefs.setBoolPref("network.cookie.thirdparty.sessionOnly", false);
  do_set_cookies(uri1, channel2, false, [1, 2, 3, 4]);
  do_set_cookies(uri2, channel1, true, [1, 2, 3, 4]);

  
  do_reload_profile(profile, cso);
  do_check_eq(cs.countCookiesFromHost(uri1.host), 4);
  do_check_eq(cs.countCookiesFromHost(uri2.host), 0);

  
  do_reload_profile(profile, cso, "shutdown-cleanse");
  do_check_eq(cs.countCookiesFromHost(uri1.host), 0);
  do_check_eq(cs.countCookiesFromHost(uri2.host), 0);

  
  prefs.setBoolPref("network.cookie.thirdparty.sessionOnly", true);
  do_set_cookies(uri1, channel2, false, [1, 2, 3, 4]);
  do_set_cookies(uri2, channel1, true, [1, 2, 3, 4]);

  
  do_reload_profile(profile, cso);
  do_check_eq(cs.countCookiesFromHost(uri1.host), 0);
  do_check_eq(cs.countCookiesFromHost(uri2.host), 0);
}

