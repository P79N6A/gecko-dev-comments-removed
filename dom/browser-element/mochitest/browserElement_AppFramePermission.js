



"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function makeAllAppsLaunchable() {
  var originalValue = SpecialPowers.setAllAppsLaunchable(true);

  
  window.addEventListener("unload", function restoreAllAppsLaunchable(event) {
    if (event.target == window.document) {
      window.removeEventListener("unload", restoreAllAppsLaunchable, false);
      SpecialPowers.setAllAppsLaunchable(originalValue);
    }
  }, false);
}
makeAllAppsLaunchable();

function testAppElement(expectAnApp, callback) {
  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;
  iframe.setAttribute('mozapp', 'http://example.org/manifest.webapp');
  iframe.addEventListener('mozbrowsershowmodalprompt', function(e) {
    is(e.detail.message == 'app', expectAnApp, e.detail.message);
    SimpleTest.executeSoon(callback);
  });
  document.body.appendChild(iframe);
  iframe.src = 'http://example.org/tests/dom/browser-element/mochitest/file_browserElement_AppFramePermission.html';
}

function runTest() {
  SpecialPowers.addPermission("embed-apps", true, document);
  testAppElement(true, function() {
    SpecialPowers.removePermission("embed-apps", document);
    testAppElement(false, function() {
      SimpleTest.finish();
    });
  });
}

addEventListener('testready', runTest);
