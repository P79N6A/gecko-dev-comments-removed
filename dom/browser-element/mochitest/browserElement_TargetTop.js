




"use strict";

SimpleTest.waitForExplicitFinish();

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

  iframe.addEventListener('mozbrowseropenwindow', function(e) {
    ok(false, 'Not expecting an openwindow event.');
  });

  iframe.addEventListener('mozbrowserlocationchange', function(e) {
    if (/file_browserElement_TargetTop.html\?2$/.test(e.detail)) {
      ok(true, 'Got the locationchange we were looking for.');
      SimpleTest.finish();
    }
  });

  document.body.appendChild(iframe);
  iframe.src = 'file_browserElement_TargetTop.html';
}

runTest();
