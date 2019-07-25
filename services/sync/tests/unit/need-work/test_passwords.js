load("fake_login_manager.js");

var loginMgr = new FakeLoginManager(fakeSampleLogins);


var passwords = loadInSandbox("resource://services-sync/engines/passwords.js");

function test_hashLoginInfo_works() {
  var pwStore = new passwords.PasswordStore();
  var fakeUserHash = pwStore._hashLoginInfo(fakeSampleLogins[0]);
  do_check_eq(typeof fakeUserHash, 'string');
  do_check_eq(fakeUserHash.length, 40);
}

function test_synccore_itemexists_works() {
  var pwStore = new passwords.PasswordStore();
  var fakeUserHash = pwStore._hashLoginInfo(fakeSampleLogins[0]);
  var psc = new passwords.PasswordSyncCore();
  
  pwStore.wrap();
  do_check_false(pwStore._itemExists("invalid guid"));
  do_check_true(pwStore._itemExists(fakeUserHash));
}
