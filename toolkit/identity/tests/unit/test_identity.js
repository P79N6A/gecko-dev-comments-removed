


"use strict";

XPCOMUtils.defineLazyModuleGetter(this, "IDService",
                                  "resource://gre/modules/identity/Identity.jsm",
                                  "IdentityService");

function test_overall() {
  do_check_neq(IDService, null);
  run_next_test();
}

function test_mock_doc() {
  do_test_pending();
  let mockedDoc = mock_doc(null, TEST_URL, function(action, params) {
    do_check_eq(action, 'coffee');
    do_test_finished();
    run_next_test();
  });

  mockedDoc.doCoffee();
}

function test_add_identity() {
  IDService.reset();

  IDService.addIdentity(TEST_USER);

  let identities = IDService.RP.getIdentitiesForSite(TEST_URL);
  do_check_eq(identities.result.length, 1);
  do_check_eq(identities.result[0], TEST_USER);

  run_next_test();
}

function test_select_identity() {
  do_test_pending();

  IDService.reset();

  let id = "ishtar@mockmyid.com";
  setup_test_identity(id, TEST_CERT, function() {
    let gotAssertion = false;
    let mockedDoc = mock_doc(null, TEST_URL, call_sequentially(
      function(action, params) {
        
        do_check_eq(action, 'ready');
        do_check_null(params);
      },
      
      function(action, params) {
        do_check_eq(action, 'login');
        do_check_neq(params, null);

        

        gotAssertion = true;
      },
      
      function(action, params) {
        do_check_eq(action, 'ready');
        do_check_null(params);

        
        do_check_true(gotAssertion);

        do_test_finished();
        run_next_test();
      }));

    
    IDService.RP.watch(mockedDoc);

    
    makeObserver("identity-request", function (aSubject, aTopic, aData) {
      
      
      
      IDService.selectIdentity(aSubject.wrappedJSObject.rpId, id);
    });

    
    IDService.RP.request(mockedDoc.id, {});
  });
}

function test_parse_good_email() {
  var parsed = IDService.parseEmail('prime-minister@jed.gov');
  do_check_eq(parsed.username, 'prime-minister');
  do_check_eq(parsed.domain, 'jed.gov');
  run_next_test();
}

function test_parse_bogus_emails() {
  do_check_eq(null, IDService.parseEmail('@evil.org'));
  do_check_eq(null, IDService.parseEmail('foo@bar@baz.com'));
  do_check_eq(null, IDService.parseEmail('you@wellsfargo.com/accounts/transfer?to=dolske&amt=all'));
  run_next_test();
}

let TESTS = [test_overall, test_mock_doc];

TESTS.push(test_add_identity);
TESTS.push(test_select_identity);
TESTS.push(test_parse_good_email);
TESTS.push(test_parse_bogus_emails);

TESTS.forEach(add_test);

function run_test() {
  run_next_test();
}
