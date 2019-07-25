


"use strict";

XPCOMUtils.defineLazyModuleGetter(this, "IDService",
                                  "resource://gre/modules/identity/Identity.jsm",
                                  "IdentityService");

function test_id_store() {
  
  
  var store = get_idstore();

  
  store.addIdentity(TEST_USER, TEST_PRIVKEY, TEST_CERT);
  do_check_neq(store.getIdentities()[TEST_USER], null);
  do_check_eq(store.getIdentities()[TEST_USER].cert, TEST_CERT);

  
  do_check_neq(store.fetchIdentity(TEST_USER), null);
  do_check_eq(store.fetchIdentity(TEST_USER).cert, TEST_CERT);

  
  store.clearCert(TEST_USER);
  do_check_neq(store.getIdentities()[TEST_USER], null);
  do_check_null(store.getIdentities()[TEST_USER].cert);

  
  store.removeIdentity(TEST_USER);
  do_check_eq(store.getIdentities()[TEST_USER], undefined);

  
  store.setLoginState(TEST_URL, true, TEST_USER);
  do_check_neq(store.getLoginState(TEST_URL), null);
  do_check_true(store.getLoginState(TEST_URL).isLoggedIn);
  do_check_eq(store.getLoginState(TEST_URL).email, TEST_USER);

  
  store.setLoginState(TEST_URL, false, TEST_USER);
  do_check_neq(store.getLoginState(TEST_URL), null);
  do_check_false(store.getLoginState(TEST_URL).isLoggedIn);

  
  do_check_eq(store.getLoginState(TEST_URL).email, TEST_USER);

  
  do_check_null(store.getLoginState(TEST_URL2));

  
  store.clearLoginState(TEST_URL);
  do_check_null(store.getLoginState(TEST_URL));
  do_check_null(store.getLoginState(TEST_URL2));

  run_next_test();
}

let TESTS = [test_id_store,];

TESTS.forEach(add_test);

function run_test() {
  run_next_test();
}
