


"use strict";

Cu.import("resource://gre/modules/Log.jsm");
Cu.import("resource://services-common/utils.js");
Cu.import("resource://services-common/hawkrequest.js");

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

  function setLanguage(lang) {
    let acceptLanguage = Cc["@mozilla.org/supports-string;1"]
                           .createInstance(Ci.nsISupportsString);
    acceptLanguage.data = lang;
    Services.prefs.setComplexValue(
      "intl.accept_languages", Ci.nsISupportsString, acceptLanguage);
  }

  let hawk = new HAWKAuthenticatedRESTRequest("https://example.com");

  Services.prefs.addObserver("intl.accept_languages", nextTest, false);
  setLanguage(languages[testCount]);

  function nextTest() {
    CommonUtils.nextTick(function() {
      if (testCount < 2) {
        do_check_eq(hawk._intl.accept_languages, languages[testCount]);

        testCount += 1;
        setLanguage(languages[testCount]);
        nextTest();
        return;
      }
      Services.prefs.removeObserver("intl.accept_languages", nextTest);
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

