


"use strict";

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://testing-common/httpd.js");

XPCOMUtils.defineLazyModuleGetter(this, "FxAccountsMgmtService",
  "resource://gre/modules/FxAccountsMgmtService.jsm",
  "FxAccountsMgmtService");

XPCOMUtils.defineLazyModuleGetter(this, "FxAccountsManager",
  "resource://gre/modules/FxAccountsManager.jsm");


const ORIGINAL_AUTH_URI = Services.prefs.getCharPref("identity.fxaccounts.auth.uri");
let { SystemAppProxy } = Cu.import("resource://gre/modules/FxAccountsMgmtService.jsm");
const ORIGINAL_SENDCUSTOM = SystemAppProxy._sendCustomEvent;
do_register_cleanup(function() {
  Services.prefs.setCharPref("identity.fxaccounts.auth.uri", ORIGINAL_AUTH_URI);
  SystemAppProxy._sendCustomEvent = ORIGINAL_SENDCUSTOM;
});


do_get_profile();


let mockSendCustomEvent = function(aEventName, aMsg) {
  Services.obs.notifyObservers({wrappedJSObject: aMsg}, aEventName, null);
};

function run_test() {
  run_next_test();
}

add_task(function test_overall() {
  do_check_neq(FxAccountsMgmtService, null);
});



add_test(function test_invalidEmailCase_signIn() {
  do_test_pending();
  let clientEmail = "greta.garbo@gmail.com";
  let canonicalEmail = "Greta.Garbo@gmail.COM";
  let attempts = 0;

  function writeResp(response, msg) {
    if (typeof msg === "object") {
      msg = JSON.stringify(msg);
    }
    response.bodyOutputStream.write(msg, msg.length);
  }

  
  
  let server = httpd_setup({
    "/account/login": function(request, response) {
      response.setHeader("Content-Type", "application/json");
      attempts += 1;

      
      if (attempts > 2) {
        response.setStatusLine(request.httpVersion, 429, "Sorry, you had your chance");
        writeResp(response, {});
        return;
      }

      let body = CommonUtils.readBytesFromInputStream(request.bodyInputStream);
      let jsonBody = JSON.parse(body);
      let email = jsonBody.email;

      
      
      if (email == canonicalEmail) {
        response.setStatusLine(request.httpVersion, 200, "Yay");
        writeResp(response, {
          uid: "your-uid",
          sessionToken: "your-sessionToken",
          keyFetchToken: "your-keyFetchToken",
          verified: true,
          authAt: 1392144866,
        });
        return;
      }

      
      
      response.setStatusLine(request.httpVersion, 400, "Incorrect email case");
      writeResp(response, {
        code: 400,
        errno: 120,
        error: "Incorrect email case",
        email: canonicalEmail,
      });
      return;
    },
  });

  
  Services.prefs.setCharPref("identity.fxaccounts.auth.uri", server.baseURI);

  
  function onMessage(subject, topic, data) {
    let message = subject.wrappedJSObject;

    switch (message.id) {
      
      
      
      
      case "signIn":
        FxAccountsMgmtService.handleEvent({
          detail: {
            id: "getAccounts",
            data: {
              method: "getAccounts",
            }
          }
        });
        break;

      
      
      
      case "getAccounts":
        Services.obs.removeObserver(onMessage, "mozFxAccountsChromeEvent");

        do_check_eq(message.data.email, canonicalEmail);

        do_test_finished();
        server.stop(run_next_test);
        break;

      
      default:
        do_throw("wat!");
        break;
    }
  }

  Services.obs.addObserver(onMessage, "mozFxAccountsChromeEvent", false);

  SystemAppProxy._sendCustomEvent = mockSendCustomEvent;

  
  FxAccountsMgmtService.handleEvent({
    detail: {
      id: "signIn",
      data: {
        method: "signIn",
        email: clientEmail,
        password: "123456",
      },
    },
  });
});

add_test(function testHandleGetAssertionError_defaultCase() {
  do_test_pending();

  FxAccountsManager.getAssertion(null).then(
    success => {
      
      ok(false);
    },
    reason => {
      equal("INVALID_AUDIENCE", reason.error);
      do_test_finished();
      run_next_test();
    }
  )
});




function httpd_setup (handlers, port=-1) {
  let server = new HttpServer();
  for (let path in handlers) {
    server.registerPathHandler(path, handlers[path]);
  }
  try {
    server.start(port);
  } catch (ex) {
    dump("ERROR starting server on port " + port + ".  Already a process listening?");
    do_throw(ex);
  }

  
  let i = server.identity;
  server.baseURI = i.primaryScheme + "://" + i.primaryHost + ":" + i.primaryPort;

  return server;
}


