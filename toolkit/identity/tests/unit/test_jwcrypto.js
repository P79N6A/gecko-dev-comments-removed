


"use strict"

Cu.import('resource://gre/modules/identity/LogUtils.jsm');

XPCOMUtils.defineLazyModuleGetter(this, "IDService",
                                  "resource://gre/modules/identity/Identity.jsm",
                                  "IdentityService");

XPCOMUtils.defineLazyModuleGetter(this, "jwcrypto",
                                  "resource://gre/modules/identity/jwcrypto.jsm");

XPCOMUtils.defineLazyServiceGetter(this,
                                   "CryptoService",
                                   "@mozilla.org/identity/crypto-service;1",
                                   "nsIIdentityCryptoService");

const RP_ORIGIN = "http://123done.org";
const INTERNAL_ORIGIN = "browserid://";

const SECOND_MS = 1000;
const MINUTE_MS = SECOND_MS * 60;
const HOUR_MS = MINUTE_MS * 60;

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
      jwcrypto.generateAssertion("fake-cert", kp, RP_ORIGIN, (err, backedAssertion) => {
        do_check_null(err);

        do_check_eq(backedAssertion.split("~").length, 2);
        do_check_eq(backedAssertion.split(".").length, 3);

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

function test_get_assertion_with_offset() {
  do_test_pending();


  
  
  let serverMsec = Date.parse("Tue Oct 31 2000 00:00:00 GMT-0800");

  
  
  let localtimeOffsetMsec = -1 * 12 * HOUR_MS;
  let localMsec = serverMsec - localtimeOffsetMsec;

  jwcrypto.generateKeyPair(
    "DS160",
    function(err, kp) {
      jwcrypto.generateAssertion("fake-cert", kp, RP_ORIGIN,
        { duration: MINUTE_MS,
          localtimeOffsetMsec: localtimeOffsetMsec,
          now: localMsec},
          function(err, backedAssertion) {
            do_check_null(err);

            
            let cert;
            let assertion;
            [cert, assertion] = backedAssertion.split("~");

            do_check_eq(cert, "fake-cert");
            do_check_eq(assertion.split(".").length, 3);

            let components = extractComponents(assertion);

            
            let exp = parseInt(components.payload.exp, 10);
            do_check_true(exp - serverMsec === MINUTE_MS);

            do_test_finished();
            run_next_test();
          }
      );
    }
  );
}

function test_assertion_lifetime() {
  do_test_pending();

  jwcrypto.generateKeyPair(
    "DS160",
    function(err, kp) {
      jwcrypto.generateAssertion("fake-cert", kp, RP_ORIGIN,
        {duration: MINUTE_MS},
        function(err, backedAssertion) {
          do_check_null(err);

          
          let cert;
          let assertion;
          [cert, assertion] = backedAssertion.split("~");

          do_check_eq(cert, "fake-cert");
          do_check_eq(assertion.split(".").length, 3);

          let components = extractComponents(assertion);

          
          let exp = parseInt(components.payload.exp, 10);
          do_check_true(Math.abs(Date.now() - exp) > 50 * SECOND_MS);
          do_check_true(Math.abs(Date.now() - exp) <= MINUTE_MS);

          do_test_finished();
          run_next_test();
        }
      );
    }
  );
}

function test_audience_encoding_bug972582() {
  let audience = "i-like-pie.com";

  jwcrypto.generateKeyPair(
    "DS160",
    function(err, kp) {
      do_check_null(err);
      jwcrypto.generateAssertion("fake-cert", kp, audience,
        function(err, backedAssertion) {
          do_check_null(err);

          let [cert, assertion] = backedAssertion.split("~");
          let components = extractComponents(assertion);
          do_check_eq(components.payload.aud, audience);

          do_test_finished();
          run_next_test();
        }
      );
    }
  );
}




function extractComponents(signedObject) {
  if (typeof(signedObject) != 'string') {
    throw new Error("malformed signature " + typeof(signedObject));
  }

  let parts = signedObject.split(".");
  if (parts.length != 3) {
    throw new Error("signed object must have three parts, this one has " + parts.length);
  }

  let headerSegment = parts[0];
  let payloadSegment = parts[1];
  let cryptoSegment = parts[2];

  let header = JSON.parse(base64UrlDecode(headerSegment));
  let payload = JSON.parse(base64UrlDecode(payloadSegment));

  
  do_check_eq(Object.keys(header).length, 1);
  do_check_true(!!header.alg);

  
  for (let field of ["exp", "aud"]) {
    do_check_true(!!payload[field]);
  }

  return {header: header,
          payload: payload,
          headerSegment: headerSegment,
          payloadSegment: payloadSegment,
          cryptoSegment: cryptoSegment};
};

let TESTS = [
  test_sanity,
  test_generate,
  test_get_assertion,
  test_get_assertion_with_offset,
  test_assertion_lifetime,
  test_audience_encoding_bug972582,
];

TESTS = TESTS.concat([test_rsa, test_dsa]);

TESTS.forEach(add_test);

function run_test() {
  run_next_test();
}
