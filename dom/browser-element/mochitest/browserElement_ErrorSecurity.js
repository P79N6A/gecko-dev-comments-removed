




"use strict";

SimpleTest.waitForExplicitFinish();

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

  iframe.addEventListener("mozbrowsererror", function(e) {
    ok(true, "Got mozbrowsererror event.");
    ok(e.detail.type, "Event's detail has a |type| param.");
    SimpleTest.finish();
  });

  iframe.src = "https://expired.example.com";
  document.body.appendChild(iframe);
}

runTest();
