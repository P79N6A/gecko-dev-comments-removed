




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

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);

  
  Services.cookies;

  
  
  
  do_check_true(do_get_cookie_file(profile).exists());
  let db = new CookieDatabaseConnection(do_get_cookie_file(profile), 4);

  for (let i = 0; i < 3000; ++i) {
    let uri = NetUtil.newURI("http://" + i + ".com/");
    Services.cookies.setCookieString(uri, null, "oh=hai; max-age=1000", null);
  }

  do_check_eq(do_count_cookies(), 3000);

  
  while (do_count_cookies_in_db(db.db) < 3000) {
    do_execute_soon(function() {
      do_run_generator(test_generator);
    });
    yield;
  }

  
  
  let file = db.db.databaseFile;
  do_check_true(file.exists());
  do_check_true(file.fileSize < 1e6);
  db.close();

  
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
  do_load_profile();

  
  for (let i = 100; i-- > 0; ) {
    let host = i.toString() + ".com";
    Services.cookiemgr.remove(host, "oh", "/", false);
  }
  for (let i = 2900; i < 3000; ++i) {
    let host = i.toString() + ".com";
    Services.cookiemgr.remove(host, "oh", "/", false);
  }

  
  do_check_eq(do_count_cookies(), 2800);

  
  do_close_profile(test_generator);
  yield;
  do_load_profile();

  
  do_check_eq(do_count_cookies(), 2800);

  
  do_close_profile(test_generator);
  yield;
  do_load_profile(test_generator);
  yield;

  
  do_check_eq(do_count_cookies(), 2800);
  for (let i = 100; i < 2900; ++i) {
    let host = i.toString() + ".com";
    do_check_eq(Services.cookiemgr.countCookiesFromHost(host), 1);
  }

  finish_test();
}

