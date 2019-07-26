




"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function runTest() {
  const innerPage = 'http://example.com/tests/dom/browser-element/mochitest/file_browserElement_CookiesNotThirdParty.html';

  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

  iframe.addEventListener('mozbrowsershowmodalprompt', function(e) {
    if (e.detail.message == 'next') {
      iframe.src = innerPage + '?step=2';
      return;
    }

    if (e.detail.message.startsWith('success:')) {
      ok(true, e.detail.message);
      return;
    }

    if (e.detail.message.startsWith('failure:')) {
      ok(false, e.detail.message);
      return;
    }

    if (e.detail.message == 'finish') {
      SimpleTest.finish();
    }
  });

  
  
  
  
  
  iframe.src = innerPage;
  document.body.appendChild(iframe);
}


addEventListener('testready', function() {
  SpecialPowers.pushPrefEnv({'set': [['network.cookie.cookieBehavior', 1]]}, runTest);
});
