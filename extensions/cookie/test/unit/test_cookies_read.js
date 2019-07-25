





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

  for (let i = 0; i < 3000; ++i) {
    let uri = NetUtil.newURI("http://" + i + ".com/");
    Services.cookies.setCookieString(uri, null, "oh=hai; max-age=1000", null);
  }

  do_check_eq(do_count_cookies(), 3000);

  
  do_close_profile(test_generator);
  yield;
  do_load_profile();

  
  do_check_eq(Services.cookiemgr.countCookiesFromHost("2000.com"), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost("abc.com"), 0);
  do_check_eq(Services.cookiemgr.countCookiesFromHost("100.com"), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost("1400.com"), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost("xyz.com"), 0);

  
  do_check_eq(do_count_cookies(), 3000);

  
  for (let i = 0; i < 3000; ++i) {
    let host = i.toString() + ".com";
    do_check_eq(Services.cookiemgr.countCookiesFromHost(host), 1);
  }

  
  do_close_profile(test_generator);
  yield;
  do_load_profile(test_generator);
  yield;

  
  for (let i = 0; i < 3000; ++i) {
    let host = i.toString() + ".com";
    do_check_eq(Services.cookiemgr.countCookiesFromHost(host), 1);
  }

  finish_test();
}

