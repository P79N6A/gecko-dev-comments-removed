


Cu.import("resource://gre/modules/FxAccountsClient.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-crypto/utils.js");

const FAKE_SESSION_TOKEN = "a0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebf";

function run_test() {
  run_next_test();
}

function deferredStop(server) {
  let deferred = Promise.defer();
  server.stop(deferred.resolve);
  return deferred.promise;
}

add_test(function test_hawk_credentials() {
  let client = new FxAccountsClient();

  let sessionToken = "a0a1a2a3a4a5a6a7a8a9aaabacadaeafb0b1b2b3b4b5b6b7b8b9babbbcbdbebf";
  let result = client._deriveHawkCredentials(sessionToken, "session");

  do_check_eq(result.id, "639503a218ffbb62983e9628be5cd64a0438d0ae81b2b9dadeb900a83470bc6b");
  do_check_eq(CommonUtils.bytesAsHex(result.key), "3a0188943837ab228fe74e759566d0e4837cbcc7494157aac4da82025b2811b2");

  run_next_test();
});

add_task(function test_authenticated_get_request() {
  let message = "{\"msg\": \"Great Success!\"}";
  let credentials = {
    id: "eyJleHBpcmVzIjogMTM2NTAxMDg5OC4x",
    key: "qTZf4ZFpAMpMoeSsX3zVRjiqmNs=",
    algorithm: "sha256"
  };
  let method = "GET";

  let server = httpd_setup({"/foo": function(request, response) {
      do_check_true(request.hasHeader("Authorization"));

      response.setStatusLine(request.httpVersion, 200, "OK");
      response.bodyOutputStream.write(message, message.length);
    }
  });

  let client = new FxAccountsClient(server.baseURI);

  let result = yield client._request("/foo", method, credentials);
  do_check_eq("Great Success!", result.msg);

  yield deferredStop(server);
});

add_task(function test_authenticated_post_request() {
  let credentials = {
    id: "eyJleHBpcmVzIjogMTM2NTAxMDg5OC4x",
    key: "qTZf4ZFpAMpMoeSsX3zVRjiqmNs=",
    algorithm: "sha256"
  };
  let method = "POST";

  let server = httpd_setup({"/foo": function(request, response) {
      do_check_true(request.hasHeader("Authorization"));

      response.setStatusLine(request.httpVersion, 200, "OK");
      response.setHeader("Content-Type", "application/json");
      response.bodyOutputStream.writeFrom(request.bodyInputStream, request.bodyInputStream.available());
    }
  });

  let client = new FxAccountsClient(server.baseURI);

  let result = yield client._request("/foo", method, credentials, {foo: "bar"});
  do_check_eq("bar", result.foo);

  yield deferredStop(server);
});

add_task(function test_500_error() {
  let message = "<h1>Ooops!</h1>";
  let method = "GET";

  let server = httpd_setup({"/foo": function(request, response) {
      response.setStatusLine(request.httpVersion, 500, "Internal Server Error");
      response.bodyOutputStream.write(message, message.length);
    }
  });

  let client = new FxAccountsClient(server.baseURI);

  try {
    yield client._request("/foo", method);
  } catch (e) {
    do_check_eq(500, e.code);
    do_check_eq("Internal Server Error", e.message);
  }

  yield deferredStop(server);
});

add_task(function test_signUp() {
  let creationMessage = JSON.stringify({
    uid: "uid",
    sessionToken: "sessionToken",
    keyFetchToken: "keyFetchToken"
  });
  let errorMessage = JSON.stringify({code: 400, errno: 101, error: "account exists"});
  let created = false;

  let server = httpd_setup({
    "/account/create": function(request, response) {
      let body = CommonUtils.readBytesFromInputStream(request.bodyInputStream);
      let jsonBody = JSON.parse(body);

      
      do_check_eq(jsonBody.email, "andré@example.org");

      if (!created) {
        do_check_eq(jsonBody.authPW, "247b675ffb4c46310bc87e26d712153abe5e1c90ef00a4784594f97ef54f2375");
        created = true;

        response.setStatusLine(request.httpVersion, 200, "OK");
        return response.bodyOutputStream.write(creationMessage, creationMessage.length);
      }

      
      response.setStatusLine(request.httpVersion, 400, "Bad request");
      return response.bodyOutputStream.write(errorMessage, errorMessage.length);
    },
  });

  let client = new FxAccountsClient(server.baseURI);
  let result = yield client.signUp('andré@example.org', 'pässwörd');
  do_check_eq("uid", result.uid);
  do_check_eq("sessionToken", result.sessionToken);
  do_check_eq("keyFetchToken", result.keyFetchToken);

  
  try {
    result = yield client.signUp('andré@example.org', 'pässwörd');
  } catch(expectedError) {
    do_check_eq(101, expectedError.errno);
  }

  yield deferredStop(server);
});

add_task(function test_signIn() {
  let sessionMessage = JSON.stringify({sessionToken: FAKE_SESSION_TOKEN});
  let errorMessage = JSON.stringify({code: 400, errno: 102, error: "doesn't exist"});
  let server = httpd_setup({
    "/account/login": function(request, response) {
      let body = CommonUtils.readBytesFromInputStream(request.bodyInputStream);
      let jsonBody = JSON.parse(body);

      if (jsonBody.email == "mé@example.com") {
        do_check_eq(jsonBody.authPW, "08b9d111196b8408e8ed92439da49206c8ecfbf343df0ae1ecefcd1e0174a8b6");
        response.setStatusLine(request.httpVersion, 200, "OK");
        return response.bodyOutputStream.write(sessionMessage, sessionMessage.length);
      }

      
      response.setStatusLine(request.httpVersion, 400, "Bad request");
      return response.bodyOutputStream.write(errorMessage, errorMessage.length);
    },
  });

  let client = new FxAccountsClient(server.baseURI);
  let result = yield client.signIn('mé@example.com', 'bigsecret');
  do_check_eq(FAKE_SESSION_TOKEN, result.sessionToken);

  
  try {
    result = yield client.signIn("yøü@bad.example.org", "nofear");
  } catch(expectedError) {
    do_check_eq(102, expectedError.errno);
  }

  yield deferredStop(server);
});

add_task(function test_signOut() {
  let signoutMessage = JSON.stringify({});
  let errorMessage = JSON.stringify({code: 400, errno: 102, error: "doesn't exist"});
  let signedOut = false;

  let server = httpd_setup({
    "/session/destroy": function(request, response) {
      if (!signedOut) {
        signedOut = true;
        do_check_true(request.hasHeader("Authorization"));
        response.setStatusLine(request.httpVersion, 200, "OK");
        return response.bodyOutputStream.write(signoutMessage, signoutMessage.length);
      }

      
      response.setStatusLine(request.httpVersion, 400, "Bad request");
      return response.bodyOutputStream.write(errorMessage, errorMessage.length);
    },
  });

  let client = new FxAccountsClient(server.baseURI);
  let result = yield client.signOut("FakeSession");
  do_check_eq(typeof result, "object");

  
  try {
    result = yield client.signOut("FakeSession");
  } catch(expectedError) {
    do_check_eq(102, expectedError.errno);
  }

  yield deferredStop(server);
});

add_task(function test_recoveryEmailStatus() {
  let emailStatus = JSON.stringify({verified: true});
  let errorMessage = JSON.stringify({code: 400, errno: 102, error: "doesn't exist"});
  let tries = 0;

  let server = httpd_setup({
    "/recovery_email/status": function(request, response) {
      do_check_true(request.hasHeader("Authorization"));

      if (tries === 0) {
        response.setStatusLine(request.httpVersion, 200, "OK");
        return response.bodyOutputStream.write(emailStatus, emailStatus.length);
      }

      
      response.setStatusLine(request.httpVersion, 400, "Bad request");
      return response.bodyOutputStream.write(errorMessage, errorMessage.length);
    },
  });

  let client = new FxAccountsClient(server.baseURI);
  let result = yield client.recoveryEmailStatus(FAKE_SESSION_TOKEN);
  do_check_eq(result.verified, true);

  
  try {
    result = yield client.recoveryEmailStatus("some bogus session");
  } catch(expectedError) {
    do_check_eq(102, expectedError.errno);
  }

  yield deferredStop(server);
});

add_task(function test_resendVerificationEmail() {
  let emptyMessage = "{}";
  let errorMessage = JSON.stringify({code: 400, errno: 102, error: "doesn't exist"});
  let tries = 0;

  let server = httpd_setup({
    "/recovery_email/resend_code": function(request, response) {
      do_check_true(request.hasHeader("Authorization"));
      if (tries === 0) {
        response.setStatusLine(request.httpVersion, 200, "OK");
        return response.bodyOutputStream.write(emptyMessage, emptyMessage.length);
      }

      
      response.setStatusLine(request.httpVersion, 400, "Bad request");
      return response.bodyOutputStream.write(errorMessage, errorMessage.length);
    },
  });

  let client = new FxAccountsClient(server.baseURI);
  let result = yield client.resendVerificationEmail(FAKE_SESSION_TOKEN);
  do_check_eq(JSON.stringify(result), emptyMessage);

  
  try {
    result = yield client.resendVerificationEmail("some bogus session");
  } catch(expectedError) {
    do_check_eq(102, expectedError.errno);
  }

  yield deferredStop(server);
});

add_task(function test_accountKeys() {
  

  let keyFetch = h("8081828384858687 88898a8b8c8d8e8f"+
                   "9091929394959697 98999a9b9c9d9e9f");

  let response = h("ee5c58845c7c9412 b11bbd20920c2fdd"+
                   "d83c33c9cd2c2de2 d66b222613364636"+
                   "c2c0f8cfbb7c6304 72c0bd88451342c6"+
                   "c05b14ce342c5ad4 6ad89e84464c993c"+
                   "3927d30230157d08 17a077eef4b20d97"+
                   "6f7a97363faf3f06 4c003ada7d01aa70");

  let kA =       h("2021222324252627 28292a2b2c2d2e2f"+
                   "3031323334353637 38393a3b3c3d3e3f");

  let wrapKB =   h("4041424344454647 48494a4b4c4d4e4f"+
                   "5051525354555657 58595a5b5c5d5e5f");

  let responseMessage = JSON.stringify({bundle: response});
  let errorMessage = JSON.stringify({code: 400, errno: 102, error: "doesn't exist"});
  let emptyMessage = "{}";
  let attempt = 0;

  let server = httpd_setup({
    "/account/keys": function(request, response) {
      do_check_true(request.hasHeader("Authorization"));
      attempt += 1;

      switch(attempt) {
        case 1:
          
          response.setStatusLine(request.httpVersion, 200, "OK");
          response.bodyOutputStream.write(responseMessage, responseMessage.length);
          break;

        case 2:
          
          response.setStatusLine(request.httpVersion, 200, "OK");
          response.bodyOutputStream.write(emptyMessage, emptyMessage.length);
          break;

        case 3:
          
          let garbage = response;
          garbage[0] = 0; 
          response.setStatusLine(request.httpVersion, 200, "OK");
          response.bodyOutputStream.write(responseMessage, responseMessage.length);
          break;

        case 4:
          
          response.setStatusLine(request.httpVersion, 400, "Bad request");
          response.bodyOutputStream.write(errorMessage, errorMessage.length);
          break;
      }
    },
  });

  let client = new FxAccountsClient(server.baseURI);

  
  let result = yield client.accountKeys(keyFetch);
  do_check_eq(CommonUtils.hexToBytes(kA), result.kA);
  do_check_eq(CommonUtils.hexToBytes(wrapKB), result.wrapKB);

  
  try {
    result = yield client.accountKeys(keyFetch);
  } catch(expectedError) {
    do_check_eq(expectedError.message, "failed to retrieve keys");
  }

  
  try {
    result = yield client.accountKeys(keyFetch);
  } catch(expectedError) {
    do_check_eq(expectedError.message, "error unbundling encryption keys");
  }

  
  try {
    result = yield client.accountKeys(keyFetch);
  } catch(expectedError) {
    do_check_eq(102, expectedError.errno);
  }

  yield deferredStop(server);
});

add_task(function test_signCertificate() {
  let certSignMessage = JSON.stringify({cert: {bar: "baz"}});
  let errorMessage = JSON.stringify({code: 400, errno: 102, error: "doesn't exist"});
  let tries = 0;

  let server = httpd_setup({
    "/certificate/sign": function(request, response) {
      do_check_true(request.hasHeader("Authorization"));

      if (tries === 0) {
        tries += 1;
        let body = CommonUtils.readBytesFromInputStream(request.bodyInputStream);
        let jsonBody = JSON.parse(body);
        do_check_eq(JSON.parse(jsonBody.publicKey).foo, "bar");
        do_check_eq(jsonBody.duration, 600);
        response.setStatusLine(request.httpVersion, 200, "OK");
        return response.bodyOutputStream.write(certSignMessage, certSignMessage.length);
      }

      
      response.setStatusLine(request.httpVersion, 400, "Bad request");
      return response.bodyOutputStream.write(errorMessage, errorMessage.length);
    },
  });

  let client = new FxAccountsClient(server.baseURI);
  let result = yield client.signCertificate(FAKE_SESSION_TOKEN, JSON.stringify({foo: "bar"}), 600);
  do_check_eq("baz", result.bar);

  
  try {
    result = yield client.signCertificate("bogus", JSON.stringify({foo: "bar"}), 600);
  } catch(expectedError) {
    do_check_eq(102, expectedError.errno);
  }

  yield deferredStop(server);
});

add_task(function test_accountExists() {
  let sessionMessage = JSON.stringify({sessionToken: FAKE_SESSION_TOKEN});
  let existsMessage = JSON.stringify({error: "wrong password", code: 400, errno: 103});
  let doesntExistMessage = JSON.stringify({error: "no such account", code: 400, errno: 102});
  let emptyMessage = "{}";

  let server = httpd_setup({
    "/account/login": function(request, response) {
      let body = CommonUtils.readBytesFromInputStream(request.bodyInputStream);
      let jsonBody = JSON.parse(body);

      switch (jsonBody.email) {
        
        case "i.exist@example.com":
        case "i.also.exist@example.com":
          response.setStatusLine(request.httpVersion, 400, "Bad request");
          response.bodyOutputStream.write(existsMessage, existsMessage.length);
          break;

        
        case "i.dont.exist@example.com":
          response.setStatusLine(request.httpVersion, 400, "Bad request");
          response.bodyOutputStream.write(doesntExistMessage, doesntExistMessage.length);
          break;

        
        
        case "i.break.things@example.com":
          response.setStatusLine(request.httpVersion, 500, "Alas");
          response.bodyOutputStream.write(emptyMessage, emptyMessage.length);
          break;

        default:
          throw new Error("Unexpected login from " + jsonBody.email);
          break;
      }
    },
  });

  let client = new FxAccountsClient(server.baseURI);
  let result;

  try {
    result = yield client.accountExists("i.exist@example.com");
  } catch(expectedError) {
    do_check_eq(expectedError.code, 400);
    do_check_eq(expectedError.errno, 103);
  }

  try {
    result = yield client.accountExists("i.also.exist@example.com");
  } catch(expectedError) {
    do_check_eq(expectedError.errno, 103);
  }

  try {
    result = yield client.accountExists("i.dont.exist@example.com");
  } catch(expectedError) {
    do_check_eq(expectedError.errno, 102);
  }

  try {
    result = yield client.accountExists("i.break.things@example.com");
  } catch(unexpectedError) {
    do_check_eq(unexpectedError.code, 500);
  }

  yield deferredStop(server);
});

add_task(function test_email_case() {
  let canonicalEmail = "greta.garbo@gmail.com";
  let clientEmail = "Greta.Garbo@gmail.COM";
  let attempts = 0;

  function writeResp(response, msg) {
    if (typeof msg === "object") {
      msg = JSON.stringify(msg);
    }
    response.bodyOutputStream.write(msg, msg.length);
  }

  let server = httpd_setup(
    {
      "/account/login": function(request, response) {
        response.setHeader("Content-Type", "application/json; charset=utf-8");
        attempts += 1;
        if (attempts > 2) {
          response.setStatusLine(request.httpVersion, 429, "Sorry, you had your chance");
          return writeResp(response, "");
        }

        let body = CommonUtils.readBytesFromInputStream(request.bodyInputStream);
        let jsonBody = JSON.parse(body);
        let email = jsonBody.email;

        
        
        if (email == canonicalEmail) {
          response.setStatusLine(request.httpVersion, 200, "Yay");
          return writeResp(response, {areWeHappy: "yes"});
        }

        response.setStatusLine(request.httpVersion, 400, "Incorrect email case");
        return writeResp(response, {
          code: 400,
          errno: 120,
          error: "Incorrect email case",
          email: canonicalEmail
        });
      },
    }
  );

  let client = new FxAccountsClient(server.baseURI);

  let result = yield client.signIn(clientEmail, "123456");
  do_check_eq(result.areWeHappy, "yes");
  do_check_eq(attempts, 2);

  yield deferredStop(server);
});


function h(hexStr) {
  return hexStr.replace(/\s+/g, "");
}
