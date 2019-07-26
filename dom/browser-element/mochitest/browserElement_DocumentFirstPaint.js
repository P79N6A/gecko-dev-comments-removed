



"use strict";

SimpleTest.waitForExplicitFinish();
var iframe;

function runTestQueue(queue) {
  if (queue.length == 0) {
    SimpleTest.finish();
    return;
  }

  var gotFirstPaint = false;
  var gotFirstLocationChange = false;
  var test = queue.shift();

  function runNext() {
    iframe.removeEventListener('mozbrowserdocumentfirstpaint', documentfirstpainthandler);
    iframe.removeEventListener('mozbrowserloadend', loadendhandler);
    runTestQueue(queue);
  }

  function documentfirstpainthandler(e) {
    ok(!gotFirstPaint, "Got firstpaint only once");
    gotFirstPaint = true;
    if (gotFirstLocationChange) {
      runNext();
    }
  }

  function loadendhandler(e) {
    gotFirstLocationChange = true;
    if (gotFirstPaint) {
      runNext();
    }
  }

  iframe.addEventListener('mozbrowserdocumentfirstpaint', documentfirstpainthandler);
  iframe.addEventListener('mozbrowserloadend', loadendhandler);

  test();
}

function testChangeLocation() {
  iframe.src = browserElementTestHelpers.emptyPage1 + "?2";
}

function testReload() {
  iframe.reload();
}

function testFirstLoad() {
  document.body.appendChild(iframe);
  iframe.src = browserElementTestHelpers.emptyPage1;
}

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  iframe = document.createElement('iframe');
  iframe.mozbrowser = true;

  runTestQueue([testFirstLoad, testReload, testChangeLocation]);
}

runTest();
