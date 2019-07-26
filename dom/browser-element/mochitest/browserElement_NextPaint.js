



"use strict";

SimpleTest.waitForExplicitFinish();

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  var iframe = document.createElement('iframe');
  iframe.mozbrowser = true;
  document.body.appendChild(iframe);

  
  iframe.addNextPaintListener(wrongListener);

  var gotFirstNextPaintEvent = false;
  iframe.addNextPaintListener(function () {
    ok(!gotFirstNextPaintEvent, 'got the first nextpaint event');

    
    gotFirstNextPaintEvent = true;

    iframe.addNextPaintListener(function () {
      info('got the second nextpaint event');
      SimpleTest.finish();
    });

    
    SimpleTest.executeSoon(function () iframe.src += '#next');
  });

  
  iframe.removeNextPaintListener(wrongListener);
  iframe.src = 'file_browserElement_NextPaint.html';
}

function wrongListener() {
  ok(false, 'first listener should have been removed');
}

runTest();
