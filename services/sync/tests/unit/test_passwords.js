Cu.import("resource://weave/util.js");

function run_test() {
  
  var passwords = loadInSandbox("resource://weave/engines/passwords.js");

  
  var fakeUser = {
    hostname: "www.boogle.com",
    formSubmitURL: "http://www.boogle.com/search",
    httpRealm: "",
    username: "",
    password: "",
    usernameField: "test_person",
    passwordField: "test_password"
    };

  Utils.getLoginManager = function fake_getLoginManager() {
    
    return {getAllLogins: function() { return [fakeUser]; }};
  };

  Utils.getProfileFile = function fake_getProfileFile(arg) {
    return {exists: function() {return false;}};
  };

  
  var fakeUserHash = passwords._hashLoginInfo(fakeUser);
  do_check_eq(typeof fakeUserHash, 'string');
  do_check_eq(fakeUserHash.length, 40);

  
  var psc = new passwords.PasswordSyncCore();
  do_check_false(psc._itemExists("invalid guid"));
  do_check_true(psc._itemExists(fakeUserHash));

  var engine = new passwords.PasswordEngine();

}
