






"use strict";

const gFxAOAuthTokenData = Cu.import("resource:///modules/loop/MozLoopService.jsm", {}).gFxAOAuthTokenData;
const BASE_URL = "http://mochi.test:8888/browser/browser/components/loop/test/mochitest/loop_fxa.sjs?";

add_task(function* setup() {
  Services.prefs.setCharPref("loop.server", BASE_URL);
  Services.prefs.setCharPref("services.push.serverURL", "ws://localhost/");
  registerCleanupFunction(function* () {
    info("cleanup time");
    yield promiseDeletedOAuthParams(BASE_URL);
    Services.prefs.clearUserPref("loop.server");
    Services.prefs.clearUserPref("services.push.serverURL");
  });
});

add_task(function* checkOAuthParams() {
  let params = {
    client_id: "client_id",
    content_uri: BASE_URL + "/content",
    oauth_uri: BASE_URL + "/oauth",
    profile_uri: BASE_URL + "/profile",
    state: "state",
  };
  yield promiseOAuthParamsSetup(BASE_URL, params);
  let client = yield MozLoopServiceInternal.promiseFxAOAuthClient();
  for (let key of Object.keys(params)) {
    ise(client.parameters[key], params[key], "Check " + key + " was passed to the OAuth client");
  }
});

add_task(function* basicAuthorization() {
  let result = yield MozLoopServiceInternal.promiseFxAOAuthAuthorization();
  is(result.code, "code1", "Check code");
  is(result.state, "state", "Check state");
});

add_task(function* sameOAuthClientForTwoCalls() {
  resetFxA();
  let client1 = yield MozLoopServiceInternal.promiseFxAOAuthClient();
  let client2 = yield MozLoopServiceInternal.promiseFxAOAuthClient();
  ise(client1, client2, "The same client should be returned");
});

add_task(function* paramsInvalid() {
  resetFxA();
  
  yield promiseDeletedOAuthParams(BASE_URL);
  let result = null;
  let loginPromise = MozLoopService.logInToFxA();
  let caught = false;
  yield loginPromise.catch(() => {
    ok(true, "The login promise should be rejected due to invalid params");
    caught = true;
  });
  ok(caught, "Should have caught the rejection");
  is(result, null, "No token data should be returned");
});

add_task(function* params_nonJSON() {
  resetFxA();
  Services.prefs.setCharPref("loop.server", "https://loop.invalid");
  let result = null;
  let loginPromise = MozLoopService.logInToFxA();
  yield loginPromise.catch(() => {
    ok(true, "The login promise should be rejected due to non-JSON params");
  });
  is(result, null, "No token data should be returned");
  Services.prefs.setCharPref("loop.server", BASE_URL);
});

add_task(function* invalidState() {
  resetFxA();
  let params = {
    client_id: "client_id",
    content_uri: BASE_URL + "/content",
    oauth_uri: BASE_URL + "/oauth",
    profile_uri: BASE_URL + "/profile",
    state: "invalid_state",
  };
  yield promiseOAuthParamsSetup(BASE_URL, params);
  let loginPromise = MozLoopService.logInToFxA();
  yield loginPromise.catch((error) => {
    ok(error, "The login promise should be rejected due to invalid state");
  });
});

add_task(function* basicRegistration() {
  resetFxA();
  let params = {
    client_id: "client_id",
    content_uri: BASE_URL + "/content",
    oauth_uri: BASE_URL + "/oauth",
    profile_uri: BASE_URL + "/profile",
    state: "state",
  };
  yield promiseOAuthParamsSetup(BASE_URL, params);

  let tokenData = yield MozLoopServiceInternal.promiseFxAOAuthToken("code1", "state");
  is(tokenData.access_token, "code1_access_token", "Check access_token");
  is(tokenData.scope, "profile", "Check scope");
  is(tokenData.token_type, "bearer", "Check token_type");
});

add_task(function* registrationWithInvalidState() {
  resetFxA();
  let params = {
    client_id: "client_id",
    content_uri: BASE_URL + "/content",
    oauth_uri: BASE_URL + "/oauth",
    profile_uri: BASE_URL + "/profile",
    state: "invalid_state",
  };
  yield promiseOAuthParamsSetup(BASE_URL, params);

  let tokenPromise = MozLoopServiceInternal.promiseFxAOAuthToken("code1", "state");
  yield tokenPromise.then(body => {
    ok(false, "Promise should have rejected");
  },
  error => {
    is(error.code, 400, "Check error code");
  });
});

add_task(function* registrationWith401() {
  resetFxA();
  let params = {
    client_id: "client_id",
    content_uri: BASE_URL + "/content",
    oauth_uri: BASE_URL + "/oauth",
    profile_uri: BASE_URL + "/profile",
    state: "state",
    test_error: "token_401",
  };
  yield promiseOAuthParamsSetup(BASE_URL, params);

  let tokenPromise = MozLoopServiceInternal.promiseFxAOAuthToken("code1", "state");
  yield tokenPromise.then(body => {
    ok(false, "Promise should have rejected");
  },
  error => {
    is(error.code, 401, "Check error code");
  });
});

add_task(function* basicAuthorizationAndRegistration() {
  resetFxA();
  let params = {
    client_id: "client_id",
    content_uri: BASE_URL + "/content",
    oauth_uri: BASE_URL + "/oauth",
    profile_uri: BASE_URL + "/profile",
    state: "state",
  };
  yield promiseOAuthParamsSetup(BASE_URL, params);

  let tokenData = yield MozLoopService.logInToFxA();
  ise(tokenData.access_token, "code1_access_token", "Check access_token");
  ise(tokenData.scope, "profile", "Check scope");
  ise(tokenData.token_type, "bearer", "Check token_type");
});

add_task(function* loginWithParams401() {
  resetFxA();
  let params = {
    client_id: "client_id",
    content_uri: BASE_URL + "/content",
    oauth_uri: BASE_URL + "/oauth",
    profile_uri: BASE_URL + "/profile",
    state: "state",
    test_error: "params_401",
  };
  yield promiseOAuthParamsSetup(BASE_URL, params);

  let loginPromise = MozLoopService.logInToFxA();
  yield loginPromise.then(tokenData => {
    ok(false, "Promise should have rejected");
  },
  error => {
    ise(error.code, 401, "Check error code");
    ise(gFxAOAuthTokenData, null, "Check there is no saved token data");
  });
});

add_task(function* loginWithRegistration401() {
  resetFxA();
  let params = {
    client_id: "client_id",
    content_uri: BASE_URL + "/content",
    oauth_uri: BASE_URL + "/oauth",
    profile_uri: BASE_URL + "/profile",
    state: "state",
    test_error: "token_401",
  };
  yield promiseOAuthParamsSetup(BASE_URL, params);

  let loginPromise = MozLoopService.logInToFxA();
  yield loginPromise.then(tokenData => {
    ok(false, "Promise should have rejected");
  },
  error => {
    ise(error.code, 401, "Check error code");
    ise(gFxAOAuthTokenData, null, "Check there is no saved token data");
  });
});
