





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

  
  let schema3db = new CookieDatabaseConnection(do_get_cookie_file(profile), 3);

  let now = Date.now() * 1000;
  let futureExpiry = Math.round(now / 1e6 + 1000);
  let pastExpiry = Math.round(now / 1e6 - 1000);

  
  
  for (let i = 0; i < 20; ++i) {
    let cookie = new Cookie("oh" + i, "hai", "foo.com", "/",
                            futureExpiry, now, now + i, false, false, false);

    schema3db.insertCookie(cookie);
  }

  
  for (let i = 20; i < 40; ++i) {
    let cookie = new Cookie("oh" + i, "hai", "bar.com", "/",
                            pastExpiry, now, now + i, false, false, false);

    schema3db.insertCookie(cookie);
  }

  
  
  for (let i = 40; i < 45; ++i) {
    let cookie = new Cookie("oh", "hai", "baz.com", "/",
                            futureExpiry + i, now, now + i, false, false, false);

    schema3db.insertCookie(cookie);
  }
  for (let i = 45; i < 50; ++i) {
    let cookie = new Cookie("oh", "hai", "baz.com", "/",
                            pastExpiry - i, now, now + i, false, false, false);

    schema3db.insertCookie(cookie);
  }
  for (let i = 50; i < 55; ++i) {
    let cookie = new Cookie("oh", "hai", "baz.com", "/",
                            futureExpiry - i, now, now + i, false, false, false);

    schema3db.insertCookie(cookie);
  }
  for (let i = 55; i < 60; ++i) {
    let cookie = new Cookie("oh", "hai", "baz.com", "/",
                            pastExpiry + i, now, now + i, false, false, false);

    schema3db.insertCookie(cookie);
  }

  
  schema3db.close();
  schema3db = null;

  
  
  
  do_check_eq(Services.cookiemgr.countCookiesFromHost("foo.com"), 20);

  
  do_check_eq(Services.cookiemgr.countCookiesFromHost("bar.com"), 20);

  
  
  do_check_eq(Services.cookiemgr.countCookiesFromHost("baz.com"), 1);
  let enumerator = Services.cookiemgr.getCookiesFromHost("baz.com");
  let cookie = enumerator.getNext().QueryInterface(Ci.nsICookie2);
  do_check_eq(cookie.expiry, futureExpiry + 44);

  do_close_profile(test_generator);
  yield;

  
  schema3db = new CookieDatabaseConnection(do_get_cookie_file(profile), 3);

  
  for (let i = 60; i < 80; ++i) {
    let cookie = new Cookie("oh" + i, "hai", "cat.com", "/",
                            futureExpiry, now, now + i, false, false, false);

    schema3db.insertCookie(cookie);
  }

  
  schema3db.close();
  schema3db = null;

  
  
  do_load_profile();

  
  do_check_eq(Services.cookiemgr.countCookiesFromHost("cat.com"), 20);
  enumerator = Services.cookiemgr.getCookiesFromHost("cat.com");
  cookie = enumerator.getNext().QueryInterface(Ci.nsICookie2);
  do_check_eq(cookie.creationTime, 0);

  finish_test();
}

