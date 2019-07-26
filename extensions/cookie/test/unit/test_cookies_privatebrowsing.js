




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

function make_channel(url) {
  var ios = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);
  var chan = ios.newChannel(url, null, null).QueryInterface(Ci.nsIHttpChannel);
  return chan;
}

function do_run_test() {
  
  let profile = do_get_profile();

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);

  
  let uri1 = NetUtil.newURI("http://foo.com/foo.html");
  let uri2 = NetUtil.newURI("http://bar.com/bar.html");

  
  Services.cookies.setCookieString(uri1, null, "oh=hai; max-age=1000", null);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 1);

  
  var chan1 = make_channel(uri1.spec);
  chan1.QueryInterface(Ci.nsIPrivateBrowsingChannel);
  chan1.setPrivate(true);

  var chan2 = make_channel(uri2.spec);
  chan2.QueryInterface(Ci.nsIPrivateBrowsingChannel);
  chan2.setPrivate(true);

  Services.cookies.setCookieString(uri2, null, "oh=hai; max-age=1000", chan2);
  do_check_eq(Services.cookiemgr.getCookieString(uri1, chan1), null);
  do_check_eq(Services.cookiemgr.getCookieString(uri2, chan2), "oh=hai");

  
  Services.obs.notifyObservers(null, "last-pb-context-exited", null);
  do_check_eq(Services.cookiemgr.getCookieString(uri1, chan1), null);
  do_check_eq(Services.cookiemgr.getCookieString(uri2, chan2), null);

  Services.cookies.setCookieString(uri2, null, "oh=hai; max-age=1000", chan2);
  do_check_eq(Services.cookiemgr.getCookieString(uri2, chan2), "oh=hai");

  
  Services.obs.notifyObservers(null, "last-pb-context-exited", null);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 0);

  
  do_close_profile(test_generator);
  yield;
  do_load_profile();

  
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 0);

  
  do_check_eq(Services.cookiemgr.getCookieString(uri1, chan1), null);
  do_check_eq(Services.cookiemgr.getCookieString(uri2, chan2), null);
  Services.cookies.setCookieString(uri2, null, "oh=hai; max-age=1000", chan2);
  do_check_eq(Services.cookiemgr.getCookieString(uri2, chan2), "oh=hai");

  
  do_close_profile(test_generator);
  yield;
  do_load_profile();

  
  
  do_check_eq(Services.cookiemgr.getCookieString(uri1, chan1), null);
  do_check_eq(Services.cookiemgr.getCookieString(uri2, chan2), null);

  
  Services.obs.notifyObservers(null, "last-pb-context-exited", null);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 0);

  

  
  do_close_profile(test_generator);
  yield;
  do_load_profile(test_generator);
  yield;

  
  
  do_check_eq(Services.cookiemgr.getCookieString(uri1, chan1), null);
  do_check_eq(Services.cookiemgr.getCookieString(uri2, chan2), null);

  
  Services.obs.notifyObservers(null, "last-pb-context-exited", null);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri1.host), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(uri2.host), 0);

  finish_test();
}
