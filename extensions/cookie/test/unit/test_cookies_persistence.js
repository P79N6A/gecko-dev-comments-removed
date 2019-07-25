







let test_generator = do_run_test();

function run_test() {
  do_test_pending();
  test_generator.next();
}

function finish_test() {
  do_execute_soon(function() {
    test_generator.close();
    do_test_finished();
  });
}

function do_run_test() {
  
  let profile = do_get_profile();

  
  
  var spec1 = "http://foo.com/foo.html";
  var spec2 = "http://bar.com/bar.html";
  var uri1 = NetUtil.newURI(spec1);
  var uri2 = NetUtil.newURI(spec2);
  var channel1 = NetUtil.newChannel(uri1);
  var channel2 = NetUtil.newChannel(uri2);

  
  
  var httpchannel1 = channel1.QueryInterface(Ci.nsIHttpChannelInternal);
  var httpchannel2 = channel1.QueryInterface(Ci.nsIHttpChannelInternal);
  httpchannel1.forceAllowThirdPartyCookie = true;
  httpchannel2.forceAllowThirdPartyCookie = true;

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);
  Services.prefs.setBoolPref("network.cookie.thirdparty.sessionOnly", false);
  do_set_cookies(uri1, channel1, false, [1, 2, 3, 4]);
  do_set_cookies(uri2, channel2, true, [1, 2, 3, 4]);

  
  do_close_profile(test_generator);
  yield;
  do_load_profile();
  do_check_eq(Services.cookies.countCookiesFromHost(uri1.host), 4);
  do_check_eq(Services.cookies.countCookiesFromHost(uri2.host), 0);

  
  
  
  do_close_profile();
  do_load_profile();
  do_check_eq(Services.cookies.countCookiesFromHost(uri1.host), 4);
  do_check_eq(Services.cookies.countCookiesFromHost(uri2.host), 0);

  
  do_close_profile(test_generator, "shutdown-cleanse");
  yield;
  do_load_profile();
  do_check_eq(Services.cookies.countCookiesFromHost(uri1.host), 0);
  do_check_eq(Services.cookies.countCookiesFromHost(uri2.host), 0);

  
  Services.prefs.setIntPref("network.cookie.lifetimePolicy", 2);
  do_set_cookies(uri1, channel1, false, [1, 2, 3, 4]);
  do_set_cookies(uri2, channel2, true, [1, 2, 3, 4]);

  
  do_close_profile(test_generator);
  yield;
  do_load_profile();
  do_check_eq(Services.cookies.countCookiesFromHost(uri1.host), 0);
  do_check_eq(Services.cookies.countCookiesFromHost(uri2.host), 0);

  finish_test();
}

