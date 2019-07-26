




"use strict";
SimpleTest.waitForExplicitFinish();

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

  var gotPopup = false;
  iframe.addEventListener('mozbrowseropenwindow', function(e) {
    is(gotPopup, false, 'Should get just one popup.');
    gotPopup = true;

    document.body.appendChild(e.detail.frameElement);

    ok(/file_browserElement_Open2\.html$/.test(e.detail.url),
       "Popup's URL (got " + e.detail.url + ")");
    is(e.detail.name, "name");
    is(e.detail.features, "dialog=1");
  });

  iframe.addEventListener('mozbrowsershowmodalprompt', function(e) {
    ok(gotPopup, 'Got mozbrowseropenwindow event before showmodalprompt event.');
    if (e.detail.message.indexOf("success:") == 0) {
      ok(true, e.detail.message);
    }
    else if (e.detail.message.indexOf("failure:") == 0) {
      ok(false, e.detail.message);
    }
    else if (e.detail.message == "finish") {
      SimpleTest.finish();
    }
    else {
      ok(false, "Got invalid message: " + e.detail.message);
    }
  });

  









  iframe.src = 'file_browserElement_Open1.html';
  document.body.appendChild(iframe);
}

runTest();
