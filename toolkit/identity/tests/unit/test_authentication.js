


"use strict";

XPCOMUtils.defineLazyModuleGetter(this, "IDService",
                                  "resource://gre/modules/identity/Identity.jsm",
                                  "IdentityService");

XPCOMUtils.defineLazyModuleGetter(this, "jwcrypto",
                                  "resource://gre/modules/identity/jwcrypto.jsm");

function test_begin_authentication_flow() {
  do_test_pending();
  let _provId = null;

  
  let mockedDoc = mock_doc(null, TEST_URL, function(action, params) {});
  IDService.RP.watch(mockedDoc);

  
  
  
  makeObserver("identity-auth", function (aSubject, aTopic, aData) {
    do_check_neq(aSubject, null);

    do_check_eq(aSubject.wrappedJSObject.provId, _provId);

    do_test_finished();
    run_next_test();
  });

  setup_provisioning(
    TEST_USER,
    function(caller) {
      _provId = caller.id;
      IDService.IDP.beginProvisioning(caller);
    }, function() {},
    {
      beginProvisioningCallback: function(email, duration_s) {

        
        IDService.IDP._doAuthentication(_provId, {idpParams:TEST_IDPPARAMS});
      }
    }
  );
}

function test_complete_authentication_flow() {
  do_test_pending();
  let _provId = null;
  let _authId = null;
  let id = TEST_USER;

  let callbacksFired = false;
  let loginStateChanged = false;
  let identityAuthComplete = false;

  
  IDService.reset();

  setup_test_identity(id, TEST_CERT, function() {
    

    get_idstore().setLoginState(TEST_URL, true, id);

    
    
    
    
    let mockedDoc = mock_doc(id, TEST_URL, call_sequentially(
      function(action, params) {
        do_check_eq(action, 'ready');
        do_check_eq(params, undefined);

        
        callbacksFired = true;
        if (loginStateChanged && identityAuthComplete) {
          do_test_finished();
          run_next_test();
        }
      }
    ));

    makeObserver("identity-auth-complete", function(aSubject, aTopic, aData) {
      identityAuthComplete = true;
      do_test_finished();
      run_next_test();
    });

    makeObserver("identity-login-state-changed", function (aSubject, aTopic, aData) {
      do_check_neq(aSubject, null);

      do_check_eq(aSubject.wrappedJSObject.rpId, mockedDoc.id);
      do_check_eq(aData, id);

      
      loginStateChanged = true;
      if (callbacksFired && identityAuthComplete) {
        do_test_finished();
        run_next_test();
      }
    });

    IDService.RP.watch(mockedDoc);

    
    setup_provisioning(
      TEST_USER,
      function(provFlow) {
        _provId = provFlow.id;

        IDService.IDP.beginProvisioning(provFlow);
      }, function() {},
      {
        beginProvisioningCallback: function(email, duration_s) {
          
          IDService.IDP._doAuthentication(_provId, {idpParams:TEST_IDPPARAMS});

          
          
          
          _authId = uuid();
          IDService.IDP.setAuthenticationFlow(_authId, _provId);

          
          authCaller.id = _authId;
          IDService.IDP._provisionFlows[_provId].caller = authCaller;
          IDService.IDP.beginAuthentication(authCaller);
        }
      }
    );
  });

  
  let authCaller = {
    doBeginAuthenticationCallback: function doBeginAuthenticationCallback(identity) {
      do_check_eq(identity, TEST_USER);
      
      IDService.IDP.completeAuthentication(_authId);
    },

    doError: function(err) {
      log("OW! My doError callback hurts!", err);
    },
  };

}

let TESTS = [];

TESTS.push(test_begin_authentication_flow);
TESTS.push(test_complete_authentication_flow);

TESTS.forEach(add_test);

function run_test() {
  run_next_test();
}
