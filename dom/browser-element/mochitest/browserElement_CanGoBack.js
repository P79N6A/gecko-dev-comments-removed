




"use strict";
SimpleTest.waitForExplicitFinish();

var iframe;
function runTest() {
  
  
  
  
  
  
  
  if (!browserElementTestHelpers.getOOPByDefaultPref()) {
    ok(false, "This test only works OOP.");
    return;
  }

  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addToWhitelist();

  iframe = document.createElement('iframe');
  iframe.mozbrowser = true;

  iframe.addEventListener('mozbrowserloadend', function loadend() {
    iframe.removeEventListener('mozbrowserloadend', loadend);
    SimpleTest.executeSoon(test2);
  });

  iframe.src = browserElementTestHelpers.emptyPage1;
  document.body.appendChild(iframe);
}

function checkCanGoBackAndForward(canGoBack, canGoForward, nextTest) {
  var seenCanGoBackResult = false;
  iframe.getCanGoBack().onsuccess = function(e) {
    is(seenCanGoBackResult, false, "onsuccess handler shouldn't be called twice.");
    seenCanGoBackResult = true;
    is(e.target.result, canGoBack);
    maybeRunNextTest();
  };

  var seenCanGoForwardResult = false;
  iframe.getCanGoForward().onsuccess = function(e) {
    is(seenCanGoForwardResult, false, "onsuccess handler shouldn't be called twice.");
    seenCanGoForwardResult = true;
    is(e.target.result, canGoForward);
    maybeRunNextTest();
  };

  function maybeRunNextTest() {
    if (seenCanGoBackResult && seenCanGoForwardResult) {
      nextTest();
    }
  }
}

function test2() {
  checkCanGoBackAndForward( false,  false, test3);
}

function test3() {
  iframe.addEventListener('mozbrowserloadend', function loadend() {
    checkCanGoBackAndForward( true,  false, SimpleTest.finish);
  });
  iframe.src = browserElementTestHelpers.emptyPage2;
}

runTest();
