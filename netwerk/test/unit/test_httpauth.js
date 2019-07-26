






Components.utils.import("resource://gre/modules/Services.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

function run_test() {
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
  
  const PRIVATE = true;
  const NOT_PRIVATE = false;
  
  try {
    var domain = {value: kEmpty}, user = {value: kEmpty}, pass = {value: kEmpty};
    
    am.setAuthIdentity(kHTTP, kHost1, kPort, kBasic, kRealm, kEmpty, kDomain, kUser, kPassword);
    
    am.getAuthIdentity(kHTTP, kHost1, kPort, kBasic, kRealm, kEmpty, domain, user, pass, NOT_PRIVATE);
    do_check_eq(domain.value, kDomain);
    do_check_eq(user.value, kUser);
    do_check_eq(pass.value, kPassword);

    
    domain = {value: kEmpty}, user = {value: kEmpty}, pass = {value: kEmpty};
    try {
      
      am.getAuthIdentity(kHTTP, kHost1, kPort, kBasic, kRealm, kEmpty, domain, user, pass, PRIVATE);
      do_throw("Auth entry should not be retrievable after entering the private browsing mode");
    } catch (e) {
      do_check_eq(domain.value, kEmpty);
      do_check_eq(user.value, kEmpty);
      do_check_eq(pass.value, kEmpty);
    }

    
    am.setAuthIdentity(kHTTP, kHost2, kPort, kBasic, kRealm, kEmpty, kDomain, kUser2, kPassword2, PRIVATE);
    
    domain = {value: kEmpty}, user = {value: kEmpty}, pass = {value: kEmpty};
    am.getAuthIdentity(kHTTP, kHost2, kPort, kBasic, kRealm, kEmpty, domain, user, pass, PRIVATE);
    do_check_eq(domain.value, kDomain);
    do_check_eq(user.value, kUser2);
    do_check_eq(pass.value, kPassword2);

    try {
      
      domain = {value: kEmpty}, user = {value: kEmpty}, pass = {value: kEmpty};
      am.getAuthIdentity(kHTTP, kHost2, kPort, kBasic, kRealm, kEmpty, domain, user, pass, NOT_PRIVATE);
      do_throw("Auth entry should not be retrievable outside of private browsing mode");
    } catch (x) {
      do_check_eq(domain.value, kEmpty);
      do_check_eq(user.value, kEmpty);
      do_check_eq(pass.value, kEmpty);
    }

    
    Services.obs.notifyObservers(null, "last-pb-context-exited", null);

    
    domain = {value: kEmpty}, user = {value: kEmpty}, pass = {value: kEmpty};
    try {
      
      am.getAuthIdentity(kHTTP, kHost2, kPort, kBasic, kRealm, kEmpty, domain, user, pass, NOT_PRIVATE);
      do_throw("Auth entry should not be retrievable after exiting the private browsing mode");
    } catch (e) {
      do_check_eq(domain.value, kEmpty);
      do_check_eq(user.value, kEmpty);
      do_check_eq(pass.value, kEmpty);
    }
    try {
      
      am.getAuthIdentity(kHTTP, kHost2, kPort, kBasic, kRealm, kEmpty, domain, user, pass, PRIVATE);
      do_throw("Auth entry should not be retrievable in private mode after exiting the private browsing mode");
    } catch (x) {
      do_check_eq(domain.value, kEmpty);
      do_check_eq(user.value, kEmpty);
      do_check_eq(pass.value, kEmpty);
    }
  } catch (e) {
    do_throw("Unexpected exception while testing HTTP auth manager: " + e);
  }
}

