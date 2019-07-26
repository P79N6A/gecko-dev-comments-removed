






function run_test() {
  
  
  var spec1 = "http://foo.com/foo.html";
  var spec2 = "http://bar.com/bar.html";
  var uri1 = NetUtil.newURI(spec1);
  var uri2 = NetUtil.newURI(spec2);
  var channel1 = NetUtil.newChannel(uri1);
  var channel2 = NetUtil.newChannel(uri2);

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);
  do_set_cookies(uri1, channel1, true, [1, 2, 3, 4]);
  Services.cookies.removeAll();
  do_set_cookies(uri1, channel2, true, [1, 2, 3, 4]);
  Services.cookies.removeAll();

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 1);
  do_set_cookies(uri1, channel1, true, [0, 0, 0, 0]);
  Services.cookies.removeAll();
  do_set_cookies(uri1, channel2, true, [0, 0, 0, 0]);
  Services.cookies.removeAll();

  
  
  var httpchannel1 = channel1.QueryInterface(Ci.nsIHttpChannelInternal);
  var httpchannel2 = channel2.QueryInterface(Ci.nsIHttpChannelInternal);
  httpchannel1.forceAllowThirdPartyCookie = true;
  httpchannel2.forceAllowThirdPartyCookie = true;

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);
  do_set_cookies(uri1, channel1, true, [1, 2, 3, 4]);
  Services.cookies.removeAll();
  do_set_cookies(uri1, channel2, true, [1, 2, 3, 4]);
  Services.cookies.removeAll();

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 1);
  do_set_cookies(uri1, channel1, true, [0, 1, 1, 2]);
  Services.cookies.removeAll();
  do_set_cookies(uri1, channel2, true, [0, 0, 0, 0]);
  Services.cookies.removeAll();

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 3);
  do_set_cookies(uri1, channel1, true, [0, 1, 2, 3]);
  Services.cookies.removeAll();
  do_set_cookies(uri1, channel2, true, [0, 0, 0, 0]);
  Services.cookies.removeAll();
  do_set_single_http_cookie(uri1, channel1, 1);
  do_set_cookies(uri1, channel2, true, [2, 3, 4, 5]);
  Services.cookies.removeAll();

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);
  var kPermissionType = "cookie";
  var ALLOW_FIRST_PARTY_ONLY = 9;
  
  Services.permissions.add(uri1, kPermissionType, ALLOW_FIRST_PARTY_ONLY);
  do_set_cookies(uri1, channel1, true, [0, 1, 1, 2]);
  Services.cookies.removeAll();
  do_set_cookies(uri1, channel2, true, [0, 0, 0, 0]);
  Services.cookies.removeAll();

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 1);
  do_set_cookies(uri1, channel1, true, [0, 1, 1, 2]);
  Services.cookies.removeAll();
  
  
  do_set_cookies(uri2, channel2, true, [0, 1, 1, 2]);
  Services.cookies.removeAll();
  do_set_cookies(uri1, channel2, true, [0, 0, 0, 0]);
  Services.cookies.removeAll();

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 3);
  do_set_cookies(uri1, channel1, true, [0, 1, 1, 2]);
  Services.cookies.removeAll();
  
  
  do_set_cookies(uri2, channel2, true, [0, 1, 2, 3]);
  Services.cookies.removeAll();
  do_set_single_http_cookie(uri2, channel2, 1);
  do_set_cookies(uri2, channel2, true, [2, 3, 4, 5]);
  Services.cookies.removeAll();
  do_set_cookies(uri1, channel2, true, [0, 0, 0, 0]);
  Services.cookies.removeAll();
  do_set_single_http_cookie(uri1, channel1, 1);
  do_set_cookies(uri1, channel2, true, [1, 1, 1, 1]);
  Services.cookies.removeAll();

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);
  var kPermissionType = "cookie";
  var LIMIT_THIRD_PARTY = 10;
  
  Services.permissions.add(uri1, kPermissionType, LIMIT_THIRD_PARTY);
  do_set_cookies(uri1, channel1, true, [0, 1, 2, 3]);
  Services.cookies.removeAll();
  do_set_cookies(uri1, channel2, true, [0, 0, 0, 0]);
  Services.cookies.removeAll();
  do_set_single_http_cookie(uri1, channel1, 1);
  do_set_cookies(uri1, channel2, true, [2, 3, 4, 5]);
  Services.cookies.removeAll();

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 1);
  do_set_cookies(uri1, channel1, true, [0, 1, 2, 3]);
  Services.cookies.removeAll();
  
  
  do_set_cookies(uri2, channel2, true, [0, 1, 1, 2]);
  Services.cookies.removeAll();
  do_set_cookies(uri1, channel2, true, [0, 0, 0, 0]);
  Services.cookies.removeAll();
  do_set_single_http_cookie(uri1, channel1, 1);
  do_set_cookies(uri1, channel2, true, [2, 3, 4, 5]);
  Services.cookies.removeAll();

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 3);
  do_set_cookies(uri1, channel1, true, [0, 1, 2, 3]);
  Services.cookies.removeAll();
  
  
  do_set_cookies(uri2, channel2, true, [0, 1, 2, 3]);
  Services.cookies.removeAll();
  do_set_single_http_cookie(uri2, channel2, 1);
  do_set_cookies(uri2, channel2, true, [2, 3, 4, 5]);
  Services.cookies.removeAll();
  do_set_cookies(uri1, channel2, true, [0, 0, 0, 0]);
  Services.cookies.removeAll();
  do_set_single_http_cookie(uri1, channel1, 1);
  do_set_cookies(uri1, channel2, true, [2, 3, 4, 5]);
  Services.cookies.removeAll();
}

