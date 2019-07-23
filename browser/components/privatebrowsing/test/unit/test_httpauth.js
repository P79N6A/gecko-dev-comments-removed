







































function run_test_on_service() {
  var pb = Cc[PRIVATEBROWSING_CONTRACT_ID].
           getService(Ci.nsIPrivateBrowsingService);

  var am = Cc["@mozilla.org/network/http-auth-manager;1"].
           getService(Ci.nsIHttpAuthManager);

  const kHost1 = "pbtest3.example.com";
  const kHost2 = "pbtest4.example.com";
  const kPort = 80;
  const kHTTP = "http";
  const kBasic = "basic";
  const kRealm = "realm";
  const kDomain = "example.com";
  const kUser = "user";
  const kUser2 = "user2";
  const kPassword = "pass";
  const kPassword2 = "pass2";
  const kEmpty = "";

  try {
    var domain = {value: kEmpty}, user = {value: kEmpty}, pass = {value: kEmpty};
    
    am.setAuthIdentity(kHTTP, kHost1, kPort, kBasic, kRealm, kEmpty, kDomain, kUser, kPassword);
    
    am.getAuthIdentity(kHTTP, kHost1, kPort, kBasic, kRealm, kEmpty, domain, user, pass);
    do_check_eq(domain.value, kDomain);
    do_check_eq(user.value, kUser);
    do_check_eq(pass.value, kPassword);
    
    pb.privateBrowsingEnabled = true;
    
    domain = {value: kEmpty}, user = {value: kEmpty}, pass = {value: kEmpty};
    try {
      
      am.getAuthIdentity(kHTTP, kHost1, kPort, kBasic, kRealm, kEmpty, domain, user, pass);
      do_throw("Auth entry should not be retrievable after entering the private browsing mode");
    } catch (e) {
      do_check_eq(domain.value, kEmpty);
      do_check_eq(user.value, kEmpty);
      do_check_eq(pass.value, kEmpty);
    }

    
    am.setAuthIdentity(kHTTP, kHost2, kPort, kBasic, kRealm, kEmpty, kDomain, kUser2, kPassword2);
    
    domain = {value: kEmpty}, user = {value: kEmpty}, pass = {value: kEmpty};
    am.getAuthIdentity(kHTTP, kHost2, kPort, kBasic, kRealm, kEmpty, domain, user, pass);
    do_check_eq(domain.value, kDomain);
    do_check_eq(user.value, kUser2);
    do_check_eq(pass.value, kPassword2);
    
    pb.privateBrowsingEnabled = false;
    
    domain = {value: kEmpty}, user = {value: kEmpty}, pass = {value: kEmpty};
    try {
      
      am.getAuthIdentity(kHTTP, kHost2, kPort, kBasic, kRealm, kEmpty, domain, user, pass);
      do_throw("Auth entry should not be retrievable after exiting the private browsing mode");
    } catch (e) {
      do_check_eq(domain.value, kEmpty);
      do_check_eq(user.value, kEmpty);
      do_check_eq(pass.value, kEmpty);
    }
  } catch (e) {
    do_throw("Unexpected exception while testing HTTP auth manager: " + e);
  }
}


function run_test() {
  run_test_on_all_services();
}
