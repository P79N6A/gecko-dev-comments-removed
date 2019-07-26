




"use strict";
SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function runTest() {
  var iframeJS = document.createElement('iframe');
  SpecialPowers.wrap(iframeJS).mozbrowser = true;

  iframeJS.addEventListener('mozbrowserloadstart', function(e) {
    ok(false, "This should not happen!");
  });

  iframeJS.addEventListener('mozbrowserloadend', function(e) {
    ok(false, "This should not happen!");
  });

  iframeJS.src = 'javascript:alert("Foo");';
  document.body.appendChild(iframeJS);

  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

  var gotPopup = false;
  iframe.addEventListener('mozbrowseropenwindow', function(e) {
    is(gotPopup, false, 'Should get just one popup.');
    gotPopup = true;

    document.body.appendChild(e.detail.frameElement);
  });

  iframe.addEventListener('mozbrowsershowmodalprompt', function(e) {
    ok(gotPopup, 'Got mozbrowseropenwindow event before showmodalprompt event.');
    if (e.detail.message.indexOf("success") == 0) {
      ok(true, e.detail.message);
      SimpleTest.finish();
    }
    else {
      ok(false, "Got invalid message: " + e.detail.message);
    }
  });

  iframe.src = 'file_browserElement_FrameWrongURI.html';
  document.body.appendChild(iframe);
}

addEventListener('testready', runTest);
