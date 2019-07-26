










"use strict";

XPCOMUtils.defineLazyModuleGetter(this, "IDService",
                                  "resource://gre/modules/identity/Identity.jsm",
                                  "IdentityService");

function test_smoke() {
  do_check_neq(IDService, null);
  run_next_test();
}

function test_identity_request() {
  
  

  do_test_pending();

  IDService.reset();

  let id = "landru@mockmyid.com";
  setup_test_identity(id, TEST_CERT, function() {
    
    
    let mockedDoc = mock_doc(null, "http://jed.gov/", function() {});

    
    IDService.RP.watch(mockedDoc);

    
    makeObserver("identity-request", function (aSubject, aTopic, aData) {
      do_check_eq(aTopic, "identity-request");
      do_check_eq(aData, null);

      
      let subj = aSubject.wrappedJSObject;
      do_check_eq(subj.privacyPolicy, "http://jed.gov/pp.html");
      do_check_eq(subj.termsOfService, "http://jed.gov/tos.html");

      do_test_finished();
      run_next_test();
    });

    let requestOptions = {
      privacyPolicy: "/pp.html",
      termsOfService: "/tos.html"
    };
    IDService.RP.request(mockedDoc.id, requestOptions);
  });

}

function test_identity_auth() {
  
  

  do_test_pending();
  let _provId = "bogus";

  
  
  let idpParams = {
    domain: "myfavoriteflan.com",
    idpParams: {
      authentication: "/foo/authenticate.html",
      provisioning: "/foo/provision.html"
    }
  };

  
  let mockedDoc = mock_doc(null, TEST_URL, function(action, params) {});
  IDService.RP.watch(mockedDoc);

  
  
  
  makeObserver("identity-auth", function (aSubject, aTopic, aData) {
    do_check_neq(aSubject, null);
    do_check_eq(aTopic, "identity-auth");
    do_check_eq(aData, "https://myfavoriteflan.com/foo/authenticate.html");

    do_check_eq(aSubject.wrappedJSObject.provId, _provId);
    do_test_finished();
    run_next_test();
  });

  
  
  
  IDService.IDP._doAuthentication(_provId, idpParams);
}

let TESTS = [
    test_smoke,
    test_identity_request,
    test_identity_auth,
  ];


TESTS.forEach(add_test);

function run_test() {
  run_next_test();
}
