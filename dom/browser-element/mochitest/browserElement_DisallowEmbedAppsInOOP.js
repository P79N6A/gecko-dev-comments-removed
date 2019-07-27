









"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

SpecialPowers.setAllAppsLaunchable(true);

function runTest() {
  var canEmbedApp = !browserElementTestHelpers.getOOPByDefaultPref();
  var iframe = document.createElement('iframe');
  iframe.setAttribute('mozbrowser', 'true');

  iframe.addEventListener('mozbrowsershowmodalprompt', function(e) {
    is(e.detail.message == 'app', canEmbedApp, e.detail.message);
    SimpleTest.finish();
  });

  document.body.appendChild(iframe);

  var context = { 'url': 'http://example.org',
                  'appId': SpecialPowers.Ci.nsIScriptSecurityManager.NO_APP_ID,
                  'isInBrowserElement': true };
  SpecialPowers.pushPermissions([
    {'type': 'browser', 'allow': 1, 'context': context},
    {'type': 'embed-apps', 'allow': 1, 'context': context}
  ], function() {
    iframe.src = 'http://example.org/tests/dom/browser-element/mochitest/file_browserElement_DisallowEmbedAppsInOOP.html';
  });
}

addEventListener('testready', runTest);
