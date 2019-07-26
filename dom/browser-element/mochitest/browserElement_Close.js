



"use strict";

SimpleTest.waitForExplicitFinish();

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;
  document.body.appendChild(iframe);

  iframe.addEventListener("mozbrowserclose", function(e) {
    ok(true, "got mozbrowserclose event.");
    SimpleTest.finish();
  });

  iframe.src = "data:text/html,<html><body><script>window.close()</scr"+"ipt></body></html>";
}

runTest();
