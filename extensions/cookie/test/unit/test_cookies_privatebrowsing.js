





let test_generator = do_run_test();

function run_test() {
  do_test_pending();
  do_run_generator(test_generator);
}

function finish_test() {
  do_execute_soon(function() {
    test_generator.close();
    do_test_finished();
  });
}

function do_run_test() {
  
  let profile = do_get_profile();

  
  try {
    Services.pb;
  } catch (e) {
    finish_test();
    return;
  }

  
  Services.prefs.setBoolPref("browser.privatebrowsing.keep_current_session",
    true);

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);

  
  let uri1 = NetUtil.newURI("http://foo.com/foo.html");
  let uri2 = NetUtil.newURI("http://bar.com/bar.html");

  
  Services.cookies.setCookieString(uri1, null, "oh=hai; max-age=1000", null);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 1);

  
  Services.pb.privateBrowsingEnabled = true;
  Services.cookies.setCookieString(uri2, null, "oh=hai; max-age=1000", null);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 0);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 1);

  
  Services.cookies.removeAll();
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 0);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 0);

  
  do_check_throws(function() {
    Services.cookiemgr.importCookies(null);
  }, Cr.NS_ERROR_NOT_AVAILABLE);

  Services.cookies.setCookieString(uri2, null, "oh=hai; max-age=1000", null);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 1);

  
  Services.pb.privateBrowsingEnabled = false;
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 0);

  
  Services.pb.privateBrowsingEnabled = true;
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 0);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 0);

  
  Services.pb.privateBrowsingEnabled = false;
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 0);

  
  do_close_profile(test_generator);
  yield;
  do_load_profile();

  
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 0);

  
  Services.pb.privateBrowsingEnabled = true;
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 0);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 0);
  Services.cookies.setCookieString(uri2, null, "oh=hai; max-age=1000", null);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 1);

  
  do_close_profile(test_generator);
  yield;
  do_load_profile();

  
  
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 0);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 0);

  
  Services.pb.privateBrowsingEnabled = false;
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 0);

  
  Services.pb.privateBrowsingEnabled = true;

  
  do_close_profile(test_generator);
  yield;
  do_load_profile(test_generator);
  yield;

  
  
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 0);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 0);

  
  Services.pb.privateBrowsingEnabled = false;
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 0);

  finish_test();
}

