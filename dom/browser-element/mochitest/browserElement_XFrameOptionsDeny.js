




"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

var initialScreenshotArrayBuffer;

function arrayBuffersEqual(a, b) {
  var x = new Int8Array(a);
  var y = new Int8Array(b);
  if (x.length != y.length) {
    return false;
  }

  for (var i = 0; i < x.length; i++) {
    if (x[i] != y[i]) {
      return false;
    }
  }

  return true;
}

function runTest() {
  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

  
  
  
  iframe.height = '1000px';

  iframe.addEventListener('mozbrowsershowmodalprompt', function(e) {
    switch (e.detail.message) {
    case 'step 1':
      
      
      e.preventDefault();

      iframe.getScreenshot(1000, 1000).onsuccess = function(sshot) {
        var fr = new FileReader();
        fr.onloadend = function() {
          initialScreenshotArrayBuffer = fr.result;
          e.detail.unblock();
        }
        fr.readAsArrayBuffer(sshot.target.result);
      };
      break;
    case 'step 2':
      
      
      iframe.getScreenshot(1000, 1000).onsuccess = function(sshot) {
        var fr = new FileReader();
        fr.onloadend = function() {
          ok(arrayBuffersEqual(fr.result, initialScreenshotArrayBuffer),
             "Screenshots should be identical");
          SimpleTest.finish();
        }
        fr.readAsArrayBuffer(sshot.target.result);
      };
      break;
    }
  });

  document.body.appendChild(iframe);

  
  
  
  iframe.src = 'http://example.com/tests/dom/browser-element/mochitest/file_browserElement_XFrameOptionsDeny.html';
}

addEventListener('testready', runTest);
