



"use strict";

SimpleTest.waitForExplicitFinish();

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

  iframe.addEventListener('mozbrowsershowmodalprompt', function(e) {
    is(e.detail.message, 'Hello');
    SimpleTest.finish();
  });

  iframe.src = 'file_browserElement_AlertInFrame.html';
  document.body.appendChild(iframe);
}

runTest();
