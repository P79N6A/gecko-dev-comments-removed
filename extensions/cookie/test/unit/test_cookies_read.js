




let test_generator = do_run_test();

let CMAX = 1000;    

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

  for (let i = 0; i < CMAX; ++i) {
    let uri = NetUtil.newURI("http://" + i + ".com/");
    Services.cookies.setCookieString(uri, null, "oh=hai; max-age=1000", null);
  }

  do_check_eq(do_count_cookies(), CMAX);

  
  while (do_count_cookies_in_db(db.db) < CMAX) {
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

  
  do_check_eq(Services.cookiemgr.countCookiesFromHost("999.com"), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost("abc.com"), 0);
  do_check_eq(Services.cookiemgr.countCookiesFromHost("100.com"), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost("400.com"), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost("xyz.com"), 0);

  
  do_check_eq(do_count_cookies(), CMAX);

  
  for (let i = 0; i < CMAX; ++i) {
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
  for (let i = CMAX - 100; i < CMAX; ++i) {
    let host = i.toString() + ".com";
    Services.cookiemgr.remove(host, "oh", "/", false);
  }

  
  do_check_eq(do_count_cookies(), CMAX - 200);

  
  do_close_profile(test_generator);
  yield;
  do_load_profile();

  
  do_check_eq(do_count_cookies(), CMAX - 200);

  
  do_close_profile(test_generator);
  yield;
  do_load_profile(test_generator);
  yield;

  
  do_check_eq(do_count_cookies(), CMAX - 200);
  for (let i = 100; i < CMAX - 100; ++i) {
    let host = i.toString() + ".com";
    do_check_eq(Services.cookiemgr.countCookiesFromHost(host), 1);
  }

  finish_test();
}

