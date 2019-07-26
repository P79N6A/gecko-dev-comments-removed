


"use strict";

Cu.import("resource://services-common/utils.js");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FxAccounts.jsm");
Cu.import("resource://gre/modules/FxAccountsClient.jsm");
Cu.import("resource://gre/modules/FxAccountsCommon.js");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Log.jsm");

const ONE_HOUR_MS = 1000 * 60 * 60;
const ONE_DAY_MS = ONE_HOUR_MS * 24;
const TWO_MINUTES_MS = 1000 * 60 * 2;

initTestLogging("Trace");


Cu.importGlobalProperties(['atob']);

let log = Log.repository.getLogger("Services.FxAccounts.test");
log.level = Log.Level.Debug;


Services.prefs.setCharPref("identity.fxaccounts.loglevel", "DEBUG");

function run_test() {
  run_next_test();
}









function MockFxAccountsClient() {
  this._email = "nobody@example.com";
  this._verified = false;

  
  
  this.recoveryEmailStatus = function (sessionToken) {
    
    let deferred = Promise.defer();

    let response = {
      email: this._email,
      verified: this._verified
    };
    deferred.resolve(response);

    return deferred.promise;
  };

  this.accountKeys = function (keyFetchToken) {
    let deferred = Promise.defer();

    do_timeout(50, () => {
      let response = {
        kA: expandBytes("11"),
        wrapKB: expandBytes("22")
      };
      deferred.resolve(response);
    });
    return deferred.promise;
  };

  this.resendVerificationEmail = function(sessionToken) {
    
    return Promise.resolve(sessionToken);
  };

  this.signCertificate = function() { throw "no" };

  FxAccountsClient.apply(this);
}
MockFxAccountsClient.prototype = {
  __proto__: FxAccountsClient.prototype
}

let MockStorage = function() {
  this.data = null;
};
MockStorage.prototype = Object.freeze({
  set: function (contents) {
    this.data = contents;
    return Promise.resolve(null);
  },
  get: function () {
    return Promise.resolve(this.data);
  },
});







function MockFxAccounts() {
  return new FxAccounts({
    _getCertificateSigned_calls: [],
    _d_signCertificate: Promise.defer(),
    _now_is: new Date(),
    signedInUserStorage: new MockStorage(),
    now: function () {
      return this._now_is;
    },
    getCertificateSigned: function (sessionToken, serializedPublicKey) {
      _("mock getCertificateSigned\n");
      this._getCertificateSigned_calls.push([sessionToken, serializedPublicKey]);
      return this._d_signCertificate.promise;
    },
    fxAccountsClient: new MockFxAccountsClient()
  });
}

add_test(function test_non_https_remote_server_uri() {
  Services.prefs.setCharPref(
    "identity.fxaccounts.remote.signup.uri",
    "http://example.com/browser/browser/base/content/test/general/accounts_testRemoteCommands.html");
  do_check_throws_message(function () {
    fxAccounts.getAccountsSignUpURI();
  }, "Firefox Accounts server must use HTTPS");

  Services.prefs.clearUserPref("identity.fxaccounts.remote.signup.uri");

  run_next_test();
});

add_task(function test_get_signed_in_user_initially_unset() {
  
  
  
  let account = new FxAccounts({onlySetInternal: true})
  let credentials = {
    email: "foo@example.com",
    uid: "1234@lcip.org",
    assertion: "foobar",
    sessionToken: "dead",
    kA: "beef",
    kB: "cafe",
    verified: true
  };

  let result = yield account.getSignedInUser();
  do_check_eq(result, null);

  yield account.setSignedInUser(credentials);

  let result = yield account.getSignedInUser();
  do_check_eq(result.email, credentials.email);
  do_check_eq(result.assertion, credentials.assertion);
  do_check_eq(result.kB, credentials.kB);

  
  
  delete account.internal.signedInUser;
  let result = yield account.getSignedInUser();
  do_check_eq(result.email, credentials.email);
  do_check_eq(result.assertion, credentials.assertion);
  do_check_eq(result.kB, credentials.kB);

  
  yield account.signOut();

  
  let result = yield account.getSignedInUser();
  do_check_eq(result, null);
});


add_test(function test_client_mock() {
  do_test_pending();

  let fxa = new MockFxAccounts();
  let client = fxa.internal.fxAccountsClient;
  do_check_eq(client._verified, false);
  do_check_eq(typeof client.signIn, "function");

  
  client.recoveryEmailStatus()
    .then(response => {
      do_check_eq(response.verified, false);
      do_test_finished();
      run_next_test();
    });
});





add_test(function test_verification_poll() {
  do_test_pending();

  let fxa = new MockFxAccounts();
  let test_user = getTestUser("francine");
  let login_notification_received = false;

  makeObserver(ONVERIFIED_NOTIFICATION, function() {
    log.debug("test_verification_poll observed onverified");
    
    fxa.internal.getUserAccountData().then(user => {
      
      do_check_eq(user.verified, true);
      do_check_eq(user.email, test_user.email);
      do_check_true(login_notification_received);
      do_test_finished();
      run_next_test();
    });
  });

  makeObserver(ONLOGIN_NOTIFICATION, function() {
    log.debug("test_verification_poll observer onlogin");
    login_notification_received = true;
  });

  fxa.setSignedInUser(test_user).then(() => {
    fxa.internal.getUserAccountData().then(user => {
      
      do_check_eq(user.verified, false);
      do_timeout(200, function() {
        log.debug("Mocking verification of francine's email");
        fxa.internal.fxAccountsClient._email = test_user.email;
        fxa.internal.fxAccountsClient._verified = true;
      });
    });
  });
});




add_test(function test_polling_timeout() {
  do_test_pending();

  
  
  
  

  let fxa = new MockFxAccounts();
  let test_user = getTestUser("carol");

  let removeObserver = makeObserver(ONVERIFIED_NOTIFICATION, function() {
    do_throw("We should not be getting a login event!");
  });

  fxa.internal.POLL_SESSION = 1;
  fxa.internal.POLL_STEP = 2;

  let p = fxa.internal.whenVerified({});

  fxa.setSignedInUser(test_user).then(() => {
    p.then(
      (success) => {
        do_throw("this should not succeed");
      },
      (fail) => {
        removeObserver();
        do_test_finished();
        run_next_test();
      }
    );
  });
});

add_test(function test_getKeys() {
  do_test_pending();
  let fxa = new MockFxAccounts();
  let user = getTestUser("eusebius");

  
  user.verified = true;

  fxa.setSignedInUser(user).then(() => {
    fxa.getSignedInUser().then((user) => {
      
      do_check_eq(!!user.kA, false);
      do_check_eq(!!user.kB, false);
      
      do_check_eq(!!user.keyFetchToken, true);

      fxa.internal.getKeys().then(() => {
        fxa.getSignedInUser().then((user) => {
          
          do_check_eq(fxa.internal.isUserEmailVerified(user), true);
          do_check_eq(!!user.verified, true);
          do_check_eq(user.kA, expandHex("11"));
          do_check_eq(user.kB, expandHex("66"));
          do_check_eq(user.keyFetchToken, undefined);
          do_test_finished();
          run_next_test();
        });
      });
    });
  });
});


add_test(function test_getKeys_no_token() {
  do_test_pending();

  let fxa = new MockFxAccounts();
  let user = getTestUser("lettuce.protheroe");
  delete user.keyFetchToken

  makeObserver(ONLOGOUT_NOTIFICATION, function() {
    log.debug("test_getKeys_no_token observed logout");
    fxa.internal.getUserAccountData().then(user => {
      do_test_finished();
      run_next_test();
    });
  });

  fxa.setSignedInUser(user).then((user) => {
    fxa.internal.getKeys();
  });
});




add_test(function test_overlapping_signins() {
  do_test_pending();

  let fxa = new MockFxAccounts();
  let alice = getTestUser("alice");
  let bob = getTestUser("bob");

  makeObserver(ONVERIFIED_NOTIFICATION, function() {
    log.debug("test_overlapping_signins observed onverified");
    
    fxa.internal.getUserAccountData().then(user => {
      do_check_eq(user.email, bob.email);
      do_check_eq(user.verified, true);
      do_test_finished();
      run_next_test();
    });
  });

  
  fxa.setSignedInUser(alice).then(() => {
    log.debug("Alice signing in ...");
    fxa.internal.getUserAccountData().then(user => {
      do_check_eq(user.email, alice.email);
      do_check_eq(user.verified, false);
      log.debug("Alice has not verified her email ...");

      
      log.debug("Bob signing in ...");
      fxa.setSignedInUser(bob).then(() => {
        do_timeout(200, function() {
          
          log.debug("Bob verifying his email ...");
          fxa.internal.fxAccountsClient._verified = true;
        });
      });
    });
  });
});

add_task(function test_getAssertion() {
  let fxa = new MockFxAccounts();

  do_check_throws(function() {
    yield fxa.getAssertion("nonaudience");
  });

  let creds = {
    sessionToken: "sessionToken",
    kA: expandHex("11"),
    kB: expandHex("66"),
    verified: true
  };
  
  
  yield fxa.setSignedInUser(creds);

  _("== ready to go\n");
  
  
  
  let now = Date.parse("Mon, 13 Jan 2014 21:45:06 GMT");
  let start = now;
  fxa.internal._now_is = now;

  let d = fxa.getAssertion("audience.example.com");
  
  _("-- back from fxa.getAssertion\n");
  fxa.internal._d_signCertificate.resolve("cert1");
  let assertion = yield d;
  do_check_eq(fxa.internal._getCertificateSigned_calls.length, 1);
  do_check_eq(fxa.internal._getCertificateSigned_calls[0][0], "sessionToken");
  do_check_neq(assertion, null);
  _("ASSERTION: " + assertion + "\n");
  let pieces = assertion.split("~");
  do_check_eq(pieces[0], "cert1");
  let keyPair = fxa.internal.currentAccountState.keyPair;
  let cert = fxa.internal.currentAccountState.cert;
  do_check_neq(keyPair, undefined);
  _(keyPair.validUntil + "\n");
  let p2 = pieces[1].split(".");
  let header = JSON.parse(atob(p2[0]));
  _("HEADER: " + JSON.stringify(header) + "\n");
  do_check_eq(header.alg, "DS128");
  let payload = JSON.parse(atob(p2[1]));
  _("PAYLOAD: " + JSON.stringify(payload) + "\n");
  do_check_eq(payload.aud, "audience.example.com");
  do_check_eq(keyPair.validUntil, start + KEY_LIFETIME);
  do_check_eq(cert.validUntil, start + CERT_LIFETIME);
  _("delta: " + Date.parse(payload.exp - start) + "\n");
  let exp = Number(payload.exp);

  do_check_eq(exp, now + ASSERTION_LIFETIME);

  
  fxa.internal._d_signCertificate = Promise.defer();

  
  
  assertion = yield fxa.getAssertion("other.example.com");

  
  do_check_eq(fxa.internal._getCertificateSigned_calls.length, 1);

  
  now += ONE_HOUR_MS;
  fxa.internal._now_is = now;

  
  
  assertion = yield fxa.getAssertion("third.example.com");

  
  pieces = assertion.split("~");
  do_check_eq(pieces[0], "cert1");
  p2 = pieces[1].split(".");
  header = JSON.parse(atob(p2[0]));
  payload = JSON.parse(atob(p2[1]));
  do_check_eq(payload.aud, "third.example.com");

  
  
  
  

  keyPair = fxa.internal.currentAccountState.keyPair;
  cert = fxa.internal.currentAccountState.cert;
  do_check_eq(keyPair.validUntil, start + KEY_LIFETIME);
  do_check_eq(cert.validUntil, start + CERT_LIFETIME);
  exp = Number(payload.exp);
  do_check_eq(exp, now + ASSERTION_LIFETIME);

  
  
  now += ONE_DAY_MS;
  fxa.internal._now_is = now;
  d = fxa.getAssertion("fourth.example.com");
  fxa.internal._d_signCertificate.resolve("cert2");
  assertion = yield d;
  do_check_eq(fxa.internal._getCertificateSigned_calls.length, 2);
  do_check_eq(fxa.internal._getCertificateSigned_calls[1][0], "sessionToken");
  pieces = assertion.split("~");
  do_check_eq(pieces[0], "cert2");
  p2 = pieces[1].split(".");
  header = JSON.parse(atob(p2[0]));
  payload = JSON.parse(atob(p2[1]));
  do_check_eq(payload.aud, "fourth.example.com");
  keyPair = fxa.internal.currentAccountState.keyPair;
  cert = fxa.internal.currentAccountState.cert;
  do_check_eq(keyPair.validUntil, now + KEY_LIFETIME);
  do_check_eq(cert.validUntil, now + CERT_LIFETIME);
  exp = Number(payload.exp);

  do_check_eq(exp, now + ASSERTION_LIFETIME);
  _("----- DONE ----\n");
});

add_task(function test_resend_email_not_signed_in() {
  let fxa = new MockFxAccounts();

  try {
    yield fxa.resendVerificationEmail();
  } catch(err) {
    do_check_eq(err.message,
      "Cannot resend verification email; no signed-in user");
    return;
  }
  do_throw("Should not be able to resend email when nobody is signed in");
});

add_test(function test_resend_email() {
  let fxa = new MockFxAccounts();
  let alice = getTestUser("alice");

  let initialState = fxa.internal.currentAccountState;

  
  fxa.setSignedInUser(alice).then(() => {
    log.debug("Alice signing in");

    
    do_check_true(fxa.internal.currentAccountState !== initialState);
    let aliceState = fxa.internal.currentAccountState;

    
    do_check_true(fxa.internal.currentTimer > 0);

    fxa.internal.getUserAccountData().then(user => {
      do_check_eq(user.email, alice.email);
      do_check_eq(user.verified, false);
      log.debug("Alice wants verification email resent");

      fxa.resendVerificationEmail().then((result) => {
        
        
        do_check_eq(result, "alice's session token");

        
        do_check_true(fxa.internal.currentAccountState === aliceState);

        
        do_check_true(fxa.internal.currentTimer > 0);

        
        fxa.internal.abortExistingFlow();
        run_next_test();
      });
    });
  });
});






function expandHex(two_hex) {
  
  let eight_hex = two_hex + two_hex + two_hex + two_hex;
  let thirtytwo_hex = eight_hex + eight_hex + eight_hex + eight_hex;
  return thirtytwo_hex + thirtytwo_hex;
};

function expandBytes(two_hex) {
  return CommonUtils.hexToBytes(expandHex(two_hex));
};

function getTestUser(name) {
  return {
    email: name + "@example.com",
    uid: "1ad7f502-4cc7-4ec1-a209-071fd2fae348",
    sessionToken: name + "'s session token",
    keyFetchToken: name + "'s keyfetch token",
    unwrapBKey: expandHex("44"),
    verified: false
  };
}

function makeObserver(aObserveTopic, aObserveFunc) {
  let observer = {
    
    
    QueryInterface: XPCOMUtils.generateQI([Ci.nsISupports, Ci.nsIObserver]),

    observe: function (aSubject, aTopic, aData) {
      log.debug("observed " + aTopic + " " + aData);
      if (aTopic == aObserveTopic) {
        removeMe();
        aObserveFunc(aSubject, aTopic, aData);
      }
    }
  };

  function removeMe() {
    log.debug("removing observer for " + aObserveTopic);
    Services.obs.removeObserver(observer, aObserveTopic);
  }

  Services.obs.addObserver(observer, aObserveTopic, false);
  return removeMe;
}

function do_check_throws(func, result, stack)
{
  if (!stack)
    stack = Components.stack.caller;

  try {
    func();
  } catch (ex) {
    if (ex.name == result) {
      return;
    }
    do_throw("Expected result " + result + ", caught " + ex, stack);
  }

  if (result) {
    do_throw("Expected result " + result + ", none thrown", stack);
  }
}
