


"use strict";

const {interfaces: Ci, results: Cr, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FxAccounts.jsm");

function run_test() {
  run_next_test();
}

let credentials = {
  email: "foo@example.com",
  uid: "1234@lcip.org",
  assertion: "foobar",
  sessionToken: "dead",
  kA: "beef",
  kB: "cafe"
};

add_test(function test_non_https_remote_server_uri() {

  Services.prefs.setCharPref("firefox.accounts.remoteUrl",
                             "http://example.com/browser/browser/base/content/test/general/accounts_testRemoteCommands.html");
  do_check_throws(function () {
    fxAccounts.getAccountsURI();
  }, "Firefox Accounts server must use HTTPS");

  Services.prefs.clearUserPref("firefox.accounts.remoteUrl");

  run_next_test();
});

add_task(function test_get_signed_in_user_initially_unset() {
  
  let result = yield fxAccounts.getSignedInUser();
  do_check_eq(result, undefined);

  
  yield fxAccounts.setSignedInUser(credentials);

  
  let result = yield fxAccounts.getSignedInUser();
  do_check_eq(result.email, credentials.email);
  do_check_eq(result.assertion, credentials.assertion);
  do_check_eq(result.kB, credentials.kB);

  
  
  delete fxAccounts._signedInUser;
  let result = yield fxAccounts.getSignedInUser();
  do_check_eq(result.email, credentials.email);
  do_check_eq(result.assertion, credentials.assertion);
  do_check_eq(result.kB, credentials.kB);

  
  yield fxAccounts.signOut();

  
  let result = yield fxAccounts.getSignedInUser();
  do_check_eq(result, undefined);

  run_next_test();
});

