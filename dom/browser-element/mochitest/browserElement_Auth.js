



"use strict";

SimpleTest.waitForExplicitFinish();

function testFail(msg) {
  ok(false, JSON.stringify(msg));
}

var iframe;

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  iframe = document.createElement('iframe');
  iframe.mozbrowser = true;
  document.body.appendChild(iframe);

  
  
  iframe.addEventListener('mozbrowserloadend', function loadend() {
    iframe.removeEventListener('mozbrowserloadend', loadend);
    iframe.addEventListener('mozbrowserusernameandpasswordrequired', testHttpAuthCancel);
    SimpleTest.executeSoon(function() {
      
      iframe.src = 'http://test/tests/dom/browser-element/mochitest/file_http_401_response.sjs';
    });
  });
}

function testHttpAuthCancel(e) {
  iframe.removeEventListener("mozbrowserusernameandpasswordrequired", testHttpAuthCancel);
  
  
  iframe.addEventListener("mozbrowserusernameandpasswordrequired", testFail);
  iframe.addEventListener("mozbrowsertitlechange", function onTitleChange(e) {
    iframe.removeEventListener("mozbrowsertitlechange", onTitleChange);
    iframe.removeEventListener("mozbrowserusernameandpasswordrequired", testFail);
    is(e.detail, 'http auth failed');
    iframe.addEventListener('mozbrowserusernameandpasswordrequired', testHttpAuth);
    SimpleTest.executeSoon(function() {
      
      iframe.src = 'http://test/tests/dom/browser-element/mochitest/file_http_401_response.sjs';
    });
  });

  is(e.detail.realm, 'http_realm');
  is(e.detail.host, 'http://test');
  e.preventDefault();

  SimpleTest.executeSoon(function() {
    e.detail.cancel();
  });
}

function testHttpAuth(e) {
  iframe.removeEventListener("mozbrowserusernameandpasswordrequired", testHttpAuth);

  
  
  iframe.addEventListener("mozbrowserusernameandpasswordrequired", testFail);
  iframe.addEventListener("mozbrowsertitlechange", function onTitleChange(e) {
    iframe.removeEventListener("mozbrowsertitlechange", onTitleChange);
    iframe.removeEventListener("mozbrowserusernameandpasswordrequired", testFail);
    is(e.detail, 'http auth success');
    SimpleTest.executeSoon(testFinish);
  });

  is(e.detail.realm, 'http_realm');
  is(e.detail.host, 'http://test');
  e.preventDefault();

  SimpleTest.executeSoon(function() {
    e.detail.authenticate("httpuser", "httppass");
  });
}

function testFinish() {
  
  var authMgr = SpecialPowers.wrap(Components)
    .classes['@mozilla.org/network/http-auth-manager;1']
    .getService(Components.interfaces.nsIHttpAuthManager);
  authMgr.clearAll();

  var pwmgr = SpecialPowers.wrap(Components)
    .classes["@mozilla.org/login-manager;1"]
    .getService(Components.interfaces.nsILoginManager);
  pwmgr.removeAllLogins();

  SimpleTest.finish();
}

runTest();
