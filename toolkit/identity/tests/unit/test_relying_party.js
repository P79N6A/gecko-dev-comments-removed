


"use strict";

XPCOMUtils.defineLazyModuleGetter(this, "RelyingParty",
                                  "resource://gre/modules/identity/RelyingParty.jsm");

function resetState() {
  get_idstore().reset();
  RelyingParty.reset();
}

function test_watch_loggedin_ready() {
  do_test_pending();

  resetState();

  let id = TEST_USER;
  setup_test_identity(id, TEST_CERT, function() {
    let store = get_idstore();

    
    store.setLoginState(TEST_URL, true, id);
    RelyingParty.watch(mock_doc(id, TEST_URL, function(action, params) {
      do_check_eq(action, 'ready');
      do_check_eq(params, undefined);

      do_test_finished();
      run_next_test();
    }));
  });
}

function test_watch_loggedin_login() {
  do_test_pending();

  resetState();

  let id = TEST_USER;
  setup_test_identity(id, TEST_CERT, function() {
    let store = get_idstore();

    
    store.setLoginState(TEST_URL, true, id);

    
    RelyingParty.watch(mock_doc(null, TEST_URL, call_sequentially(
      function(action, params) {
        do_check_eq(action, 'login');
        do_check_neq(params, null);
      },
      function(action, params) {
        do_check_eq(action, 'ready');
        do_check_null(params);

        do_test_finished();
        run_next_test();
      }
    )));
  });
}

function test_watch_loggedin_logout() {
  do_test_pending();

  resetState();

  let id = TEST_USER;
  let other_id = "otherid@foo.com";
  setup_test_identity(other_id, TEST_CERT, function() {
    setup_test_identity(id, TEST_CERT, function() {
      let store = get_idstore();

      
      
      store.setLoginState(TEST_URL, true, id);

      
      
      RelyingParty.watch(mock_doc(other_id, TEST_URL, call_sequentially(
        function(action, params) {
          do_check_eq(action, 'login');
          do_check_neq(params, null);
        },
        function(action, params) {
          do_check_eq(action, 'ready');
          do_check_null(params);

          do_test_finished();
          run_next_test();
        }
      )));
    });
  });
}

function test_watch_notloggedin_ready() {
  do_test_pending();

  resetState();

  RelyingParty.watch(mock_doc(null, TEST_URL, function(action, params) {
    do_check_eq(action, 'ready');
    do_check_eq(params, undefined);

    do_test_finished();
    run_next_test();
  }));
}

function test_watch_notloggedin_logout() {
  do_test_pending();

  resetState();

  RelyingParty.watch(mock_doc(TEST_USER, TEST_URL, call_sequentially(
    function(action, params) {
      do_check_eq(action, 'logout');
      do_check_eq(params, undefined);

      let store = get_idstore();
      do_check_null(store.getLoginState(TEST_URL));
    },
    function(action, params) {
      do_check_eq(action, 'ready');
      do_check_eq(params, undefined);
      do_test_finished();
      run_next_test();
    }
  )));
}

function test_request() {
  do_test_pending();

  
  let mockedDoc = mock_doc(null, TEST_URL, function(action, params) {
    
    
  });

  RelyingParty.watch(mockedDoc);

  
  makeObserver("identity-request", function (aSubject, aTopic, aData) {
    do_check_neq(aSubject, null);

    do_check_eq(aSubject.wrappedJSObject.rpId, mockedDoc.id);

    do_test_finished();
    run_next_test();
  });

  RelyingParty.request(mockedDoc.id, {});
}




function test_request_forceAuthentication() {
  do_test_pending();

  let mockedDoc = mock_doc(null, TEST_URL, function(action, params) {});

  RelyingParty.watch(mockedDoc);

  makeObserver("identity-request", function(aSubject, aTopic, aData) {
    do_check_eq(aSubject.wrappedJSObject.rpId, mockedDoc.id);
    do_check_eq(aSubject.wrappedJSObject.forceAuthentication, true);
    do_test_finished();
    run_next_test();
  });

  RelyingParty.request(mockedDoc.id, {forceAuthentication: true});
}




function test_request_forceIssuer() {
  do_test_pending();

  let mockedDoc = mock_doc(null, TEST_URL, function(action, params) {});

  RelyingParty.watch(mockedDoc);

  makeObserver("identity-request", function(aSubject, aTopic, aData) {
    do_check_eq(aSubject.wrappedJSObject.rpId, mockedDoc.id);
    do_check_eq(aSubject.wrappedJSObject.issuer, "https://ozten.co.uk");
    do_test_finished();
    run_next_test();
  });

  RelyingParty.request(mockedDoc.id, {issuer: "https://ozten.co.uk"});
}
function test_logout() {
  do_test_pending();

  resetState();

  let id = TEST_USER;
  setup_test_identity(id, TEST_CERT, function() {
    let store = get_idstore();

    
    store.setLoginState(TEST_URL, true, id);

    let doLogout;
    let mockedDoc = mock_doc(id, TEST_URL, call_sequentially(
      function(action, params) {
        do_check_eq(action, 'ready');
        do_check_eq(params, undefined);

        do_timeout(100, doLogout);
      },
      function(action, params) {
        do_check_eq(action, 'logout');
        do_check_eq(params, undefined);
      },
      function(action, params) {
        do_check_eq(action, 'ready');
        do_check_eq(params, undefined);

        do_test_finished();
        run_next_test();
      }));

    doLogout = function() {
      RelyingParty.logout(mockedDoc.id);
      do_check_false(store.getLoginState(TEST_URL).isLoggedIn);
      do_check_eq(store.getLoginState(TEST_URL).email, TEST_USER);
    };

    RelyingParty.watch(mockedDoc);
  });
}

let TESTS = [
  test_watch_loggedin_ready,
  test_watch_loggedin_login,
  test_watch_loggedin_logout,
  test_watch_notloggedin_ready,
  test_watch_notloggedin_logout,
  test_request,
  test_request_forceAuthentication,
  test_request_forceIssuer,
  test_logout
];

TESTS.forEach(add_test);

function run_test() {
  run_next_test();
}
