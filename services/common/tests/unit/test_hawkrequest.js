


"use strict";

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-common/hawkrequest.js");


let SESSION_KEYS = {
  sessionToken: h("a0a1a2a3a4a5a6a7 a8a9aaabacadaeaf"+
                  "b0b1b2b3b4b5b6b7 b8b9babbbcbdbebf"),

  tokenID:      h("c0a29dcf46174973 da1378696e4c82ae"+
                  "10f723cf4f4d9f75 e39f4ae3851595ab"),

  reqHMACkey:   h("9d8f22998ee7f579 8b887042466b72d5"+
                  "3e56ab0c094388bf 65831f702d2febc0"),
};

function do_register_cleanup() {
  Services.prefs.resetUserPrefs();

  
  let hawk = new HAWKAuthenticatedRESTRequest("https://example.com");
  hawk._intl.uninit();
}

function run_test() {
  Log.repository.getLogger("Services.Common.RESTRequest").level =
    Log.Level.Trace;
  initTestLogging("Trace");

  run_next_test();
}


add_test(function test_intl_accept_language() {
  let testCount = 0;
  let languages = [
    "zu-NP;vo",     
    "fa-CG;ik",     
  ];

  function setLanguagePref(lang) {
    let acceptLanguage = Cc["@mozilla.org/supports-string;1"]
                           .createInstance(Ci.nsISupportsString);
    acceptLanguage.data = lang;
    Services.prefs.setComplexValue(
      "intl.accept_languages", Ci.nsISupportsString, acceptLanguage);
  }

  let hawk = new HAWKAuthenticatedRESTRequest("https://example.com");

  Services.prefs.addObserver("intl.accept_languages", checkLanguagePref, false);
  setLanguagePref(languages[testCount]);

  function checkLanguagePref() {
    var _done = false;
    CommonUtils.nextTick(function() {
      
      do_check_true(testCount < languages.length);

      do_check_eq(hawk._intl.accept_languages, languages[testCount]);

      testCount++;
      if (testCount < languages.length) {
        
        setLanguagePref(languages[testCount]);
        return;
      }

      
      do_print("Checked " + testCount + " languages. Removing checkLanguagePref as pref observer.");
      Services.prefs.removeObserver("intl.accept_languages", checkLanguagePref);
      run_next_test();
      return;
    });
  }
});

add_test(function test_hawk_authenticated_request() {
  let onProgressCalled = false;
  let postData = {your: "data"};

  
  
  
  let then = 34329600000;

  let clockSkew = 120000;
  let timeOffset = -1 * clockSkew;
  let localTime = then + clockSkew;

  
  let acceptLanguage = Cc['@mozilla.org/supports-string;1'].createInstance(Ci.nsISupportsString);
  acceptLanguage.data = 'zu-NP'; 
  Services.prefs.setComplexValue('intl.accept_languages', Ci.nsISupportsString, acceptLanguage);

  let credentials = {
    id: "eyJleHBpcmVzIjogMTM2NTAxMDg5OC4x",
    key: "qTZf4ZFpAMpMoeSsX3zVRjiqmNs=",
    algorithm: "sha256"
  };

  let server = httpd_setup({
    "/elysium": function(request, response) {
      do_check_true(request.hasHeader("Authorization"));

      
      
      
      let authorization = request.getHeader("Authorization");
      let tsMS = parseInt(/ts="(\d+)"/.exec(authorization)[1], 10) * 1000;
      do_check_eq(tsMS, then);

      
      
      
      
      
      let lang = request.getHeader("Accept-Language");
      do_check_eq(lang, acceptLanguage);

      let message = "yay";
      response.setStatusLine(request.httpVersion, 200, "OK");
      response.bodyOutputStream.write(message, message.length);
    }
  });

  function onProgress() {
    onProgressCalled = true;
  }

  function onComplete(error) {
    do_check_eq(200, this.response.status);
    do_check_eq(this.response.body, "yay");
    do_check_true(onProgressCalled);

    Services.prefs.resetUserPrefs();
    let pref = Services.prefs.getComplexValue(
      "intl.accept_languages", Ci.nsIPrefLocalizedString);
    do_check_neq(acceptLanguage.data, pref.data);

    server.stop(run_next_test);
  }

  let url = server.baseURI + "/elysium";
  let extra = {
    now: localTime,
    localtimeOffsetMsec: timeOffset
  };

  let request = new HAWKAuthenticatedRESTRequest(url, credentials, extra);

  
  CommonUtils.nextTick(function() {
    request.post(postData, onComplete, onProgress);
  });
});

add_test(function test_hawk_language_pref_changed() {
  let languages = [
    "zu-NP",        
    "fa-CG",        
  ];

  let credentials = {
    id: "eyJleHBpcmVzIjogMTM2NTAxMDg5OC4x",
    key: "qTZf4ZFpAMpMoeSsX3zVRjiqmNs=",
    algorithm: "sha256",
  };

  function setLanguage(lang) {
    let acceptLanguage = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
    acceptLanguage.data = lang;
    Services.prefs.setComplexValue("intl.accept_languages", Ci.nsISupportsString, acceptLanguage);
  }

  let server = httpd_setup({
    "/foo": function(request, response) {
      do_check_eq(languages[1], request.getHeader("Accept-Language"));

      response.setStatusLine(request.httpVersion, 200, "OK");
    },
  });

  let url = server.baseURI + "/foo";
  let postData = {};
  let request;

  setLanguage(languages[0]);

  
  
  request = new HAWKAuthenticatedRESTRequest(url, credentials);
  CommonUtils.nextTick(testFirstLanguage);

  function testFirstLanguage() {
    do_check_eq(languages[0], request._intl.accept_languages);

    
    setLanguage(languages[1]); 
    CommonUtils.nextTick(testRequest);
  }

  function testRequest() {
    
    
    request = new HAWKAuthenticatedRESTRequest(url, credentials);
    request.post({}, function(error) {
      do_check_null(error);
      do_check_eq(200, this.response.status);

      Services.prefs.resetUserPrefs();

      server.stop(run_next_test);
    });
  }
});

add_task(function test_deriveHawkCredentials() {
  let credentials = deriveHawkCredentials(
    SESSION_KEYS.sessionToken, "sessionToken");

  do_check_eq(credentials.algorithm, "sha256");
  do_check_eq(credentials.id, SESSION_KEYS.tokenID);
  do_check_eq(CommonUtils.bytesAsHex(credentials.key), SESSION_KEYS.reqHMACkey);
});


function h(hexStr) {
  return hexStr.replace(/\s+/g, "");
}
