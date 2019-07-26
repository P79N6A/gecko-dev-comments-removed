



















let COOKIE_DATABASE_SCHEMA_CURRENT = 5;

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

  
  this.cookieFile = profile.clone();
  cookieFile.append("cookies.sqlite");
  this.backupFile = profile.clone();
  backupFile.append("cookies.sqlite.bak");
  do_check_false(cookieFile.exists());
  do_check_false(backupFile.exists());

  
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

  this.sub_generator = run_test_3(test_generator, 99);
  sub_generator.next();
  yield;

  this.sub_generator = run_test_3(test_generator, COOKIE_DATABASE_SCHEMA_CURRENT);
  sub_generator.next();
  yield;

  this.sub_generator = run_test_3(test_generator, 4);
  sub_generator.next();
  yield;

  this.sub_generator = run_test_3(test_generator, 3);
  sub_generator.next();
  yield;

  this.sub_generator = run_test_4_exists(test_generator, 1,
    "ALTER TABLE moz_cookies ADD lastAccessed INTEGER");
  sub_generator.next();
  yield;

  this.sub_generator = run_test_4_exists(test_generator, 2,
    "ALTER TABLE moz_cookies ADD baseDomain TEXT");
  sub_generator.next();
  yield;

  this.sub_generator = run_test_4_baseDomain(test_generator);
  sub_generator.next();
  yield;

  this.sub_generator = run_test_4_exists(test_generator, 3,
    "ALTER TABLE moz_cookies ADD creationTime INTEGER");
  sub_generator.next();
  yield;

  this.sub_generator = run_test_4_exists(test_generator, 3,
    "CREATE UNIQUE INDEX moz_uniqueid ON moz_cookies (name, host, path)");
  sub_generator.next();
  yield;

  finish_test();
  return;
}

const garbage = "hello thar!";

function create_garbage_file(file)
{
  
  file.create(Ci.nsIFile.NORMAL_FILE_TYPE, -1);
  do_check_true(file.exists());
  do_check_eq(file.fileSize, 0);

  
  let ostream = Cc["@mozilla.org/network/file-output-stream;1"].
                createInstance(Ci.nsIFileOutputStream);
  ostream.init(file, -1, -1, 0);
  ostream.write(garbage, garbage.length);
  ostream.flush();
  ostream.close();

  file = file.clone(); 
  do_check_eq(file.fileSize, garbage.length);
}

function check_garbage_file(file)
{
  do_check_true(file.exists());
  do_check_eq(file.fileSize, garbage.length);
  file.remove(false);
  do_check_false(file.exists());
}

function run_test_1(generator)
{
  
  create_garbage_file(cookieFile);

  
  let uri = NetUtil.newURI("http://foo.com/");
  Services.cookies.setCookieString(uri, null, "oh=hai; max-age=1000", null);

  
  do_close_profile(sub_generator);
  yield;
  do_load_profile();

  
  
  do_check_eq(do_count_cookies(), 1);
  check_garbage_file(backupFile);

  
  do_close_profile(sub_generator);
  yield;

  
  cookieFile.remove(false);
  do_check_false(cookieFile.exists());
  do_run_generator(generator);
}

function run_test_2(generator)
{
  
  do_load_profile();
  let uri = NetUtil.newURI("http://foo.com/");
  Services.cookies.setCookieString(uri, null, "oh=hai; max-age=1000", null);

  
  do_close_profile(sub_generator);
  yield;

  
  let db = Services.storage.openDatabase(cookieFile);
  db.executeSimpleSQL("DROP TABLE moz_cookies");
  db.close();

  
  do_load_profile();
  do_check_eq(do_count_cookies(), 0);
  do_check_false(backupFile.exists());

  
  do_close_profile(sub_generator);
  yield;

  
  cookieFile.remove(false);
  do_check_false(cookieFile.exists());
  do_run_generator(generator);
}

function run_test_3(generator, schema)
{
  
  
  let schema2db = new CookieDatabaseConnection(do_get_cookie_file(profile), 2);
  schema2db.insertCookie(cookie);
  schema2db.db.schemaVersion = schema;
  schema2db.close();

  
  do_load_profile();
  do_check_eq(do_count_cookies(), 0);

  
  do_close_profile(sub_generator);
  yield;

  
  let db = Services.storage.openDatabase(cookieFile);
  do_check_eq(db.schemaVersion, COOKIE_DATABASE_SCHEMA_CURRENT);
  db.close();

  
  cookieFile.remove(false);
  do_check_false(cookieFile.exists());
  do_run_generator(generator);
}

function run_test_4_exists(generator, schema, stmt)
{
  
  let db = new CookieDatabaseConnection(do_get_cookie_file(profile), schema);
  db.insertCookie(cookie);
  db.db.executeSimpleSQL(stmt);
  db.close();

  
  do_load_profile();
  do_check_eq(do_count_cookies(), 0);

  
  do_close_profile(sub_generator);
  yield;

  
  db = Services.storage.openDatabase(cookieFile);
  do_check_eq(db.schemaVersion, COOKIE_DATABASE_SCHEMA_CURRENT);
  db.close();
  do_check_true(backupFile.exists());

  
  cookieFile.remove(false);
  backupFile.remove(false);
  do_check_false(cookieFile.exists());
  do_check_false(backupFile.exists());
  do_run_generator(generator);
}

function run_test_4_baseDomain(generator)
{
  
  let db = new CookieDatabaseConnection(do_get_cookie_file(profile), 2);
  let badCookie = new Cookie("oh", "hai", ".", "/", this.futureExpiry, this.now,
    this.now, false, false, false);
  db.insertCookie(badCookie);
  db.close();

  
  do_load_profile();
  do_check_eq(do_count_cookies(), 0);

  
  do_close_profile(sub_generator);
  yield;

  
  db = Services.storage.openDatabase(cookieFile);
  do_check_eq(db.schemaVersion, COOKIE_DATABASE_SCHEMA_CURRENT);
  db.close();
  do_check_true(backupFile.exists());

  
  cookieFile.remove(false);
  backupFile.remove(false);
  do_check_false(cookieFile.exists());
  do_check_false(backupFile.exists());
  do_run_generator(generator);
}
