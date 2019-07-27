



















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
  
  this.profile = do_get_profile();

  
  Services.prefs.setIntPref("network.cookie.cookieBehavior", 0);

  
  do_check_false(do_get_cookie_file(profile).exists());
  do_check_false(do_get_backup_file(profile).exists());

  
  this.now = Date.now() * 1000;
  this.futureExpiry = Math.round(this.now / 1e6 + 1000);
  this.cookie = new Cookie("oh", "hai", "bar.com", "/", this.futureExpiry,
    this.now, this.now, false, false, false);

  this.sub_generator = run_test_1(test_generator);
  sub_generator.next();
  yield;

  this.sub_generator = run_test_2(test_generator);
  sub_generator.next();
  yield;

  this.sub_generator = run_test_3(test_generator);
  sub_generator.next();
  yield;

  this.sub_generator = run_test_4(test_generator);
  sub_generator.next();
  yield;

  this.sub_generator = run_test_5(test_generator);
  sub_generator.next();
  yield;

  finish_test();
  return;
}

function do_get_backup_file(profile)
{
  let file = profile.clone();
  file.append("cookies.sqlite.bak");
  return file;
}

function do_get_rebuild_backup_file(profile)
{
  let file = profile.clone();
  file.append("cookies.sqlite.bak-rebuild");
  return file;
}

function do_corrupt_db(file)
{
  
  
  let size = file.fileSize;
  do_check_true(size > 450e3);

  
  
  
  
  
  
  
  
  let ostream = Cc["@mozilla.org/network/file-output-stream;1"].
                createInstance(Ci.nsIFileOutputStream);
  ostream.init(file, 2, -1, 0);
  let sstream = ostream.QueryInterface(Ci.nsISeekableStream);
  let n = size - 450e3 + 20e3;
  sstream.seek(Ci.nsISeekableStream.NS_SEEK_SET, size - n);
  for (let i = 0; i < n; ++i) {
    ostream.write("a", 1);
  }
  ostream.flush();
  ostream.close();

  do_check_eq(file.clone().fileSize, size);
  return size;
}

function run_test_1(generator)
{
  
  let uri = NetUtil.newURI("http://foo.com/");
  Services.cookies.setCookieString(uri, null, "oh=hai; max-age=1000", null);

  
  do_close_profile(sub_generator);
  yield;

  
  
  
  
  
  let db2 = new CookieDatabaseConnection(do_get_cookie_file(profile), 2);
  db2.db.executeSimpleSQL("INSERT INTO moz_cookies (baseDomain) VALUES (NULL)");
  db2.close();
  let db = new CookieDatabaseConnection(do_get_cookie_file(profile), 4);
  do_check_eq(do_count_cookies_in_db(db.db), 2);

  
  do_load_profile(sub_generator);
  yield;

  
  while (do_count_cookies_in_db(db.db) == 2) {
    do_execute_soon(function() {
      do_run_generator(sub_generator);
    });
    yield;
  }
  do_check_eq(do_count_cookies_in_db(db.db), 1);

  
  db.insertCookie(cookie);
  db.close();

  
  Services.cookiemgr.add(cookie.host, cookie.path, cookie.name, "hallo",
    cookie.isSecure, cookie.isHttpOnly, cookie.isSession, cookie.expiry);

  
  do_check_eq(Services.cookiemgr.countCookiesFromHost(cookie.host), 1);

  
  new _observer(sub_generator, "cookie-db-rebuilding");
  yield;
  do_execute_soon(function() { do_run_generator(sub_generator); });
  yield;

  
  do_check_eq(Services.cookiemgr.countCookiesFromHost("foo.com"), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost(cookie.host), 1);
  do_check_eq(do_count_cookies(), 2);

  
  do_close_profile(sub_generator);
  yield;

  
  
  do_check_true(do_get_backup_file(profile).exists());
  let backupdb = Services.storage.openDatabase(do_get_backup_file(profile));
  do_check_eq(do_count_cookies_in_db(backupdb, "foo.com"), 1);
  backupdb.close();

  
  do_load_profile();

  do_check_eq(Services.cookiemgr.countCookiesFromHost("foo.com"), 1);
  let enumerator = Services.cookiemgr.getCookiesFromHost(cookie.host);
  do_check_true(enumerator.hasMoreElements());
  let dbcookie = enumerator.getNext().QueryInterface(Ci.nsICookie2);
  do_check_eq(dbcookie.value, "hallo");
  do_check_false(enumerator.hasMoreElements());

  
  do_close_profile(sub_generator);
  yield;

  
  do_get_cookie_file(profile).remove(false);
  do_get_backup_file(profile).remove(false);
  do_check_false(do_get_cookie_file(profile).exists());
  do_check_false(do_get_backup_file(profile).exists());
  do_run_generator(generator);
}

function run_test_2(generator)
{
  
  do_load_profile();
  for (let i = 0; i < 3000; ++i) {
    let uri = NetUtil.newURI("http://" + i + ".com/");
    Services.cookies.setCookieString(uri, null, "oh=hai; max-age=1000", null);
  }

  
  do_close_profile(sub_generator);
  yield;

  
  let size = do_corrupt_db(do_get_cookie_file(profile));

  
  do_load_profile();

  
  
  do_check_false(do_get_backup_file(profile).exists());

  
  
  do_check_eq(Services.cookiemgr.countCookiesFromHost("0.com"), 1);

  
  
  new _observer(sub_generator, "cookie-db-rebuilding");
  yield;
  do_execute_soon(function() { do_run_generator(sub_generator); });
  yield;

  
  do_check_eq(Services.cookiemgr.countCookiesFromHost("0.com"), 1);
  do_check_eq(do_count_cookies(), 1);

  
  do_close_profile(sub_generator);
  yield;

  
  do_check_true(do_get_backup_file(profile).exists());
  do_check_eq(do_get_backup_file(profile).fileSize, size);
  let db = Services.storage.openDatabase(do_get_cookie_file(profile));
  do_check_eq(do_count_cookies_in_db(db, "0.com"), 1);
  db.close();

  
  do_load_profile();
  do_check_eq(Services.cookiemgr.countCookiesFromHost("0.com"), 1);
  do_check_eq(do_count_cookies(), 1);

  
  do_close_profile(sub_generator);
  yield;

  
  do_get_cookie_file(profile).remove(false);
  do_get_backup_file(profile).remove(false);
  do_check_false(do_get_cookie_file(profile).exists());
  do_check_false(do_get_backup_file(profile).exists());
  do_run_generator(generator);
}

function run_test_3(generator)
{
  
  
  Services.prefs.setIntPref("network.cookie.maxPerHost", 3000);

  
  do_load_profile();
  for (let i = 0; i < 10; ++i) {
    let uri = NetUtil.newURI("http://hither.com/");
    Services.cookies.setCookieString(uri, null, "oh" + i + "=hai; max-age=1000",
      null);
  }
  for (let i = 10; i < 3000; ++i) {
    let uri = NetUtil.newURI("http://haithur.com/");
    Services.cookies.setCookieString(uri, null, "oh" + i + "=hai; max-age=1000",
      null);
  }

  
  do_close_profile(sub_generator);
  yield;

  
  let size = do_corrupt_db(do_get_cookie_file(profile));

  
  do_load_profile();

  
  
  do_check_false(do_get_backup_file(profile).exists());

  
  
  
  do_check_eq(Services.cookiemgr.countCookiesFromHost("hither.com"), 10);
  do_check_eq(Services.cookiemgr.countCookiesFromHost("haithur.com"), 0);

  
  do_check_false(do_get_backup_file(profile).exists());
  new _observer(sub_generator, "cookie-db-rebuilding");
  yield;
  do_execute_soon(function() { do_run_generator(sub_generator); });
  yield;

  
  do_close_profile(sub_generator);
  yield;
  let db = Services.storage.openDatabase(do_get_cookie_file(profile));
  do_check_eq(do_count_cookies_in_db(db, "hither.com"), 10);
  do_check_eq(do_count_cookies_in_db(db), 10);
  db.close();

  
  do_check_true(do_get_backup_file(profile).exists());
  do_check_eq(do_get_backup_file(profile).fileSize, size);

  
  do_get_backup_file(profile).moveTo(null, "cookies.sqlite");
  do_load_profile();

  
  
  do_check_false(do_get_backup_file(profile).exists());

  
  do_check_eq(do_count_cookies(), 0);

  
  do_check_false(do_get_backup_file(profile).exists());
  new _observer(sub_generator, "cookie-db-rebuilding");
  yield;
  do_execute_soon(function() { do_run_generator(sub_generator); });
  yield;

  
  do_close_profile(sub_generator);
  yield;
  db = Services.storage.openDatabase(do_get_cookie_file(profile));
  do_check_eq(do_count_cookies_in_db(db), 0);
  db.close();

  
  do_check_true(do_get_backup_file(profile).exists());
  do_check_eq(do_get_backup_file(profile).fileSize, size);

  
  do_get_cookie_file(profile).remove(false);
  do_get_backup_file(profile).remove(false);
  do_check_false(do_get_cookie_file(profile).exists());
  do_check_false(do_get_backup_file(profile).exists());
  do_run_generator(generator);
}

function run_test_4(generator)
{
  
  do_load_profile();
  for (let i = 0; i < 3000; ++i) {
    let uri = NetUtil.newURI("http://" + i + ".com/");
    Services.cookies.setCookieString(uri, null, "oh=hai; max-age=1000", null);
  }

  
  do_close_profile(sub_generator);
  yield;

  
  let size = do_corrupt_db(do_get_cookie_file(profile));

  
  do_load_profile();

  
  
  do_check_false(do_get_backup_file(profile).exists());

  
  
  do_check_eq(Services.cookiemgr.countCookiesFromHost("0.com"), 1);

  
  
  let uri = NetUtil.newURI("http://0.com/");
  Services.cookies.setCookieString(uri, null, "oh2=hai; max-age=1000", null);

  
  
  
  new _observer(sub_generator, "cookie-db-rebuilding");
  yield;
  do_execute_soon(function() { do_run_generator(sub_generator); });
  yield;

  
  do_close_profile(sub_generator);
  yield;

  
  do_check_true(do_get_backup_file(profile).exists());
  do_check_eq(do_get_backup_file(profile).fileSize, size);
  let db = Services.storage.openDatabase(do_get_cookie_file(profile));
  do_check_eq(do_count_cookies_in_db(db, "0.com"), 2);
  db.close();

  
  do_load_profile();
  do_check_eq(Services.cookiemgr.countCookiesFromHost("0.com"), 2);
  do_check_eq(do_count_cookies(), 2);

  
  do_close_profile(sub_generator);
  yield;

  
  do_get_cookie_file(profile).remove(false);
  do_get_backup_file(profile).remove(false);
  do_check_false(do_get_cookie_file(profile).exists());
  do_check_false(do_get_backup_file(profile).exists());
  do_run_generator(generator);
}

function run_test_4(generator)
{
  
  do_load_profile();
  for (let i = 0; i < 3000; ++i) {
    let uri = NetUtil.newURI("http://" + i + ".com/");
    Services.cookies.setCookieString(uri, null, "oh=hai; max-age=1000", null);
  }

  
  do_close_profile(sub_generator);
  yield;

  
  let size = do_corrupt_db(do_get_cookie_file(profile));

  
  do_load_profile();

  
  
  do_check_false(do_get_backup_file(profile).exists());

  
  
  do_check_eq(Services.cookiemgr.countCookiesFromHost("0.com"), 1);

  
  
  let uri = NetUtil.newURI("http://0.com/");
  Services.cookies.setCookieString(uri, null, "oh2=hai; max-age=1000", null);

  
  
  
  new _observer(sub_generator, "cookie-db-rebuilding");
  yield;
  do_execute_soon(function() { do_run_generator(sub_generator); });
  yield;

  
  do_check_eq(Services.cookiemgr.countCookiesFromHost("0.com"), 2);
  do_check_eq(do_count_cookies(), 2);

  
  do_close_profile(sub_generator);
  yield;

  
  do_check_true(do_get_backup_file(profile).exists());
  do_check_eq(do_get_backup_file(profile).fileSize, size);
  let db = Services.storage.openDatabase(do_get_cookie_file(profile));
  do_check_eq(do_count_cookies_in_db(db, "0.com"), 2);
  db.close();

  
  do_load_profile();
  do_check_eq(Services.cookiemgr.countCookiesFromHost("0.com"), 2);
  do_check_eq(do_count_cookies(), 2);

  
  do_close_profile(sub_generator);
  yield;

  
  do_get_cookie_file(profile).remove(false);
  do_get_backup_file(profile).remove(false);
  do_check_false(do_get_cookie_file(profile).exists());
  do_check_false(do_get_backup_file(profile).exists());
  do_run_generator(generator);
}

function run_test_5(generator)
{
  
  do_load_profile();
  let uri = NetUtil.newURI("http://bar.com/");
  Services.cookies.setCookieString(uri, null, "oh=hai; path=/; max-age=1000",
    null);
  for (let i = 0; i < 3000; ++i) {
    let uri = NetUtil.newURI("http://" + i + ".com/");
    Services.cookies.setCookieString(uri, null, "oh=hai; max-age=1000", null);
  }

  
  do_close_profile(sub_generator);
  yield;

  
  let size = do_corrupt_db(do_get_cookie_file(profile));

  
  do_load_profile();

  
  
  do_check_false(do_get_backup_file(profile).exists());

  
  
  
  do_check_eq(Services.cookiemgr.countCookiesFromHost("bar.com"), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost("0.com"), 1);

  
  
  new _observer(sub_generator, "cookie-db-rebuilding");
  yield;

  
  
  do_check_eq(Services.cookiemgr.countCookiesFromHost("bar.com"), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost("0.com"), 1);
  do_check_eq(do_count_cookies(), 2);
  do_check_true(do_get_backup_file(profile).exists());
  do_check_eq(do_get_backup_file(profile).fileSize, size);
  do_check_false(do_get_rebuild_backup_file(profile).exists());

  
  
  let db = new CookieDatabaseConnection(do_get_cookie_file(profile), 4);
  db.insertCookie(cookie);
  do_check_eq(do_count_cookies_in_db(db.db, "bar.com"), 1);
  do_check_eq(do_count_cookies_in_db(db.db), 1);
  db.close();

  
  new _observer(sub_generator, "cookie-db-closed");
  yield;

  
  do_check_true(do_get_rebuild_backup_file(profile).exists());
  do_check_true(do_get_backup_file(profile).exists());
  do_check_eq(do_get_backup_file(profile).fileSize, size);
  do_check_false(do_get_cookie_file(profile).exists());

  
  
  db = new CookieDatabaseConnection(do_get_rebuild_backup_file(profile), 4);
  do_check_eq(do_count_cookies_in_db(db.db, "bar.com"), 1);
  let count = do_count_cookies_in_db(db.db);
  do_check_true(count == 1 ||
    count == 2 && do_count_cookies_in_db(db.db, "0.com") == 1);
  db.close();

  do_check_eq(Services.cookiemgr.countCookiesFromHost("bar.com"), 1);
  do_check_eq(Services.cookiemgr.countCookiesFromHost("0.com"), 1);
  do_check_eq(do_count_cookies(), 2);

  
  
  do_close_profile();

  
  do_get_backup_file(profile).remove(false);
  do_get_rebuild_backup_file(profile).remove(false);
  do_check_false(do_get_cookie_file(profile).exists());
  do_check_false(do_get_backup_file(profile).exists());
  do_check_false(do_get_rebuild_backup_file(profile).exists());
  do_run_generator(generator);
}

