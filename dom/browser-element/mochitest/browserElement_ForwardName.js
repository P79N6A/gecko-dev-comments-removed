





"use strict";

SimpleTest.waitForExplicitFinish();

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  var iframe = document.createElement('iframe');
  iframe.mozbrowser = true;
  iframe.setAttribute('name', 'foo');

  iframe.addEventListener("mozbrowseropenwindow", function(e) {
    ok(false, 'Got mozbrowseropenwindow, but should not have.');
  });

  iframe.addEventListener('mozbrowserlocationchange', function(e) {
    ok(true, "Got locationchange to " + e.detail);
    if (e.detail.endsWith("ForwardName.html#finish")) {
      SimpleTest.finish();
    }
  });

  
  
  iframe.addEventListener('mozbrowsershowmodalprompt', function(e) {
    ok(e.detail.message.startsWith('success:'), e.detail.message);
  });

  document.body.appendChild(iframe);

  
  
  
  iframe.src = 'file_browserElement_ForwardName.html';
}

runTest();
