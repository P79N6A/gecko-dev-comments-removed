



"use strict";

SimpleTest.waitForExplicitFinish();

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;
  document.body.appendChild(iframe);

  iframe.addEventListener("mozbrowserscroll", function(e) {
    ok(true, "got mozbrowserscroll event.");
    ok(e.detail, "event.detail is not null.");
    ok(e.detail.top === 4000, "top position is correct.");
    ok(e.detail.left === 4000, "left position is correct.");
    SimpleTest.finish();
  });

  iframe.src = "data:text/html,<html><body style='min-height: 5000px; min-width: 5000px;'></body><script>window.scrollTo(4000, 4000);</script></html>";
}

runTest();
