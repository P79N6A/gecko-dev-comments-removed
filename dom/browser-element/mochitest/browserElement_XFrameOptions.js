




"use strict";

SimpleTest.waitForExplicitFinish();

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

  
  iframe.addEventListener('mozbrowsershowmodalprompt', function(e) {
    ok(true, "Got alert");
    SimpleTest.finish();
  });

  document.body.appendChild(iframe);
  iframe.src = 'file_browserElement_XFrameOptions.sjs?DENY';
}

runTest();
