


"use strict"

Cu.import('resource://gre/modules/identity/LogUtils.jsm');

XPCOMUtils.defineLazyModuleGetter(this, "IDService",
                                  "resource://gre/modules/identity/Identity.jsm",
                                  "IdentityService");

XPCOMUtils.defineLazyModuleGetter(this, "jwcrypto",
                                  "resource://gre/modules/identity/jwcrypto.jsm");

const RP_ORIGIN = "http://123done.org";
const INTERNAL_ORIGIN = "browserid://";

function test_sanity() {
  do_test_pending();

  jwcrypto.generateKeyPair("DS160", function(err, kp) {
    do_check_null(err);

    do_test_finished();
    run_next_test();
  });
}

function test_generate() {
  do_test_pending();
  jwcrypto.generateKeyPair("DS160", function(err, kp) {
    do_check_null(err);
    do_check_neq(kp, null);

    do_test_finished();
    run_next_test();
  });
}

function test_get_assertion() {
  do_test_pending();

  jwcrypto.generateKeyPair(
    "DS160",
    function(err, kp) {
      jwcrypto.generateAssertion("fake-cert", kp, RP_ORIGIN, function(err, assertion) {
        do_check_null(err);

        
        log("assertion", assertion);

        do_test_finished();
        run_next_test();
      });
    });
}

function test_rsa() {
  do_test_pending();
  function checkRSA(err, kpo) {
    do_check_neq(kpo, undefined);
    log(kpo.serializedPublicKey);
    let pk = JSON.parse(kpo.serializedPublicKey);
    do_check_eq(pk.algorithm, "RS");
















    do_test_finished();
    run_next_test();
  };

  jwcrypto.generateKeyPair("RS256", checkRSA);
}

function test_dsa() {
  do_test_pending();
  function checkDSA(err, kpo) {
    do_check_neq(kpo, undefined);
    log(kpo.serializedPublicKey);
    let pk = JSON.parse(kpo.serializedPublicKey);
    do_check_eq(pk.algorithm, "DS");

















    do_test_finished();
    run_next_test();
  };

  jwcrypto.generateKeyPair("DS160", checkDSA);
}

var TESTS = [test_sanity, test_generate, test_get_assertion];

TESTS = TESTS.concat([test_rsa, test_dsa]);

TESTS.forEach(add_test);

function run_test() {
  run_next_test();
}
