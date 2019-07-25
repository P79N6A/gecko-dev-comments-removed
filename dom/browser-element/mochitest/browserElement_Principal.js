




"use strict";

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addToWhitelist();

  var iframe = document.createElement('iframe');
  iframe.mozbrowser = true;
  document.body.appendChild(iframe);

  if (!iframe.contentWindow) {
    ok(true, "OOP, can't access contentWindow.");
    return;
  }

  SimpleTest.waitForExplicitFinish();

  
  
  checkCantReadLocation(iframe);
  SimpleTest.executeSoon(function() {
    checkCantReadLocation(iframe);
    SimpleTest.finish();
  });
}

function checkCantReadLocation(iframe) {
  try {
    if (iframe.contentWindow.location == 'foo') {
      ok(false, 'not reached');
    }
    ok(false, 'should have gotten exception');
  }
  catch(e) {
    ok(true, 'got exception reading contentWindow.location');
  }
}

runTest();
