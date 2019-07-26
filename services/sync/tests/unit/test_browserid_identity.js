


Cu.import("resource://gre/modules/FxAccounts.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://services-sync/browserid_identity.js");
Cu.import("resource://services-sync/rest.js");
Cu.import("resource://services-sync/util.js");

let mockUser = {email: 'email',
                kA: 'kA',
                kB: 'kB',
                sessionToken: 'sessionToken',
                uid: 'user_uid',
               };

let _MockFXA = function(blob) {
  this.user = blob;
};
_MockFXA.prototype = {
  getSignedInUser: function getSignedInUser() {
    return Promise.resolve(this.user);
  },
  whenVerified: function whenVerified(userData) {
    return Promise.resolve(this.user);
  },
  getAssertion: function getAssertion(audience) {
    return Promise.resolve("assertion");
  },
};
let mockFXA = new _MockFXA(mockUser);

let mockToken = {
  api_endpoint: Svc.Prefs.get("services.sync.tokenServerURI"),
  duration: 300,
  id: "id",
  key: "key",
  uid: "token_uid",
};
let mockTSC = { 
  getTokenFromBrowserIDAssertion: function(uri, assertion, cb) {
    cb(null, mockToken);
  },
};

let browseridManager = new BrowserIDManager(mockFXA, mockTSC);


browseridManager._account = mockUser.email;

function run_test() {
  initTestLogging("Trace");
  Log.repository.getLogger("Sync.Identity").level = Log.Level.Trace;
  run_next_test();
};

add_test(function test_initial_state() {
    _("Verify initial state");
    do_check_false(!!browseridManager._token);
    do_check_false(browseridManager.hasValidToken());
    run_next_test();
  }
);

add_test(function test_getResourceAuthenticator() {
    _("BrowserIDManager supplies a Resource Authenticator callback which returns a Hawk header.");
    let authenticator = browseridManager.getResourceAuthenticator();
    do_check_true(!!authenticator);
    let req = {uri: CommonUtils.makeURI(
      "https://example.net/somewhere/over/the/rainbow"),
               method: 'GET'};
    let output = authenticator(req, 'GET');
    do_check_true('headers' in output);
    do_check_true('authorization' in output.headers);
    do_check_true(output.headers.authorization.startsWith('Hawk'));
    _("Expected internal state after successful call.");
    do_check_eq(browseridManager._token.uid, mockToken.uid);
    do_check_eq(browseridManager.account, mockUser.email);
    run_next_test();
  }
);

add_test(function test_getRESTRequestAuthenticator() {
    _("BrowserIDManager supplies a REST Request Authenticator callback which sets a Hawk header on a request object.");
    let request = new SyncStorageRequest(
      "https://example.net/somewhere/over/the/rainbow");
    let authenticator = browseridManager.getRESTRequestAuthenticator();
    do_check_true(!!authenticator);
    let output = authenticator(request, 'GET');
    do_check_eq(request.uri, output.uri);
    do_check_true(output._headers.authorization.startsWith('Hawk'));
    do_check_true(output._headers.authorization.contains('nonce'));
    do_check_true(browseridManager.hasValidToken());
    run_next_test();
  }
);

add_test(function test_tokenExpiration() {
    _("BrowserIDManager notices token expiration:");
    let bimExp = new BrowserIDManager(mockFXA, mockTSC);
    bimExp._account = mockUser.email;

    let authenticator = bimExp.getResourceAuthenticator();
    do_check_true(!!authenticator);
    let req = {uri: CommonUtils.makeURI(
      "https://example.net/somewhere/over/the/rainbow"),
               method: 'GET'};
    authenticator(req, 'GET');

    
    _("Forcing the token to expire ...");
    Object.defineProperty(bimExp, "_now", {
      value: function customNow() {
        return (Date.now() + 3000001);
      },
      writable: true,
    });
    do_check_true(bimExp._token.expiration < bimExp._now());
    _("... means BrowserIDManager knows to re-fetch it on the next call.");
    do_check_false(bimExp.hasValidToken());
    run_next_test();
  }
);

add_test(function test_userChangeAndLogOut() {
    _("BrowserIDManager notices when the FxAccounts.getSignedInUser().email changes.");
    let mockFXA2 = new _MockFXA(mockUser);
    let bidUser = new BrowserIDManager(mockFXA2, mockTSC);
    bidUser._account = mockUser.email;
    let request = new SyncStorageRequest(
      "https://example.net/somewhere/over/the/rainbow");
    let authenticator = bidUser.getRESTRequestAuthenticator();
    do_check_true(!!authenticator);
    let output = authenticator(request, 'GET');
    do_check_true(!!output);
    do_check_eq(bidUser.account, mockUser.email);
    do_check_true(bidUser.hasValidToken());
    mockUser.email = "something@new";
    do_check_false(bidUser.hasValidToken());
    run_next_test();
  }
);
