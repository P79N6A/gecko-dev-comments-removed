




"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();
SpecialPowers.addPermission("embed-apps", true, window.document);

addEventListener('unload', function() {
  SpecialPowers.removePermission("embed-apps", window.document);
});

function runTest() {
  
  
  
  

  var appFrame = document.createElement('iframe');
  appFrame.setAttribute('mozbrowser', 'true');
  appFrame.setAttribute('mozapp', 'http://example.org/manifest.webapp');

  var browserFrame = document.createElement('iframe');
  browserFrame.setAttribute('mozbrowser', 'true');

  var gotAppFrameClose = false;
  appFrame.addEventListener('mozbrowserclose', function() {
    ok(true, "Got close from app frame.");
    gotAppFrameClose = true;
  });

  var gotAppFrameAlert = false;
  appFrame.addEventListener('mozbrowsershowmodalprompt', function() {
    ok(gotAppFrameClose, "Should have gotten app frame close by now.");
    ok(!gotAppFrameAlert, "Just one alert from the app frame.");
    gotAppFrameAlert = true;
    if (gotBrowserFrameAlert && gotAppFrameAlert) {
      SimpleTest.finish();
    }
  });

  browserFrame.addEventListener('mozbrowserclose', function() {
    ok(false, "Got close from browser frame.");
  });

  var gotBrowserFrameAlert = false;
  browserFrame.addEventListener('mozbrowsershowmodalprompt', function() {
    ok(!gotBrowserFrameAlert, "Just one browser frame alert.");
    gotBrowserFrameAlert = true;
    if (gotBrowserFrameAlert && gotAppFrameAlert) {
      SimpleTest.finish();
    }
  });

  document.body.appendChild(appFrame);
  document.body.appendChild(browserFrame);

  appFrame.src = 'http://example.org/tests/dom/browser-element/mochitest/file_browserElement_CloseApp.html';
  browserFrame.src = 'http://example.org/tests/dom/browser-element/mochitest/file_browserElement_CloseApp.html';
}





addEventListener('testready', function() {
  SpecialPowers.pushPrefEnv({'set': [['dom.allow_scripts_to_close_windows', false]]}, runTest);
});
