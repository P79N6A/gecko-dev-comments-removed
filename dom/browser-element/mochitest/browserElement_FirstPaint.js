



"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function runTest() {
  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

  var gotFirstPaint = false;
  var gotFirstLocationChange = false;
  var secondTestPage = 'file_browserElement_FirstPaint.html';
  iframe.addEventListener('mozbrowserfirstpaint', function(e) {
    ok(!gotFirstPaint, "Got only one first paint.");
    gotFirstPaint = true;

    is(e.detail.backgroundColor, "transparent", "Got the expected background color.");

    if (gotFirstLocationChange) {
      iframe.src = secondTestPage;
    }
  });

  iframe.addEventListener('mozbrowserlocationchange', function(e) {
    if (e.detail == browserElementTestHelpers.emptyPage1) {
      gotFirstLocationChange = true;
      if (gotFirstPaint) {
        iframe.src = secondTestPage;
      }
    }
    else if (e.detail == secondTestPage) {
      is(e.detail.backgroundColor, "rgb(0,255,0)", "Got the expected background color.");
      SimpleTest.finish();
    }
  });

  document.body.appendChild(iframe);

  iframe.src = browserElementTestHelpers.emptyPage1;
}

addEventListener('testready', runTest);
