





"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function runTest() {
  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;
  document.body.appendChild(iframe);

  iframe.addEventListener("mozbrowsershowmodalprompt", function(e) {
    document.body.removeChild(iframe);
    SimpleTest.executeSoon(function() {
      ok(true);
      SimpleTest.finish();
    });
  });

  iframe.src = "data:text/html,<html><body><script>alert(\"test\")</script>" +
               "</body></html>";
}

addEventListener('testready', runTest);
