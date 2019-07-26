




"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function runTest() {
  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

  iframe.addEventListener('mozbrowseropenwindow', function(e) {
    is(e.detail.url, 'http://example.com/');
    SimpleTest.finish();
  });

  iframe.src = "file_browserElement_TargetBlank.html";
  document.body.appendChild(iframe);
}

addEventListener('testready', runTest);
