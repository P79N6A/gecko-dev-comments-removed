




"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

var iframe;
var numPendingTests = 0;




function checkScreenshotResult(expectSuccess, args) {
  var req;
  try {
    req = iframe.getScreenshot.apply(iframe, args);
  }
  catch(e) {
    ok(!expectSuccess, "getScreenshot(" + JSON.stringify(args) + ") threw an exception.");
    return;
  }

  numPendingTests++;
  req.onsuccess = function() {
    ok(expectSuccess, "getScreenshot(" + JSON.stringify(args) + ") succeeded.");
    numPendingTests--;
    if (numPendingTests == 0) {
      SimpleTest.finish();
    }
  };

  
  req.onerror = function() {
    ok(false, "getScreenshot(" + JSON.stringify(args) + ") ran onerror.");
    numPendingTests--;
    if (numPendingTests == 0) {
      SimpleTest.finish();
    }
  };
}

function runTest() {
  iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;
  document.body.appendChild(iframe);
  iframe.src = 'data:text/html,<html>' +
    '<body style="background:green">hello</body></html>';

  iframe.addEventListener('mozbrowserfirstpaint', function() {
    
    checkScreenshotResult(true, [100, 100]);

    
    checkScreenshotResult(false, []);
    checkScreenshotResult(false, [100]);
    checkScreenshotResult(false, ['a', 100]);
    checkScreenshotResult(false, [100, 'a']);
    checkScreenshotResult(false, [-1, 100]);
    checkScreenshotResult(false, [100, -1]);

    if (numPendingTests == 0) {
      SimpleTest.finish();
    }
  });
}

addEventListener('testready', runTest);
