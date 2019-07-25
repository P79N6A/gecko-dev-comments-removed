




"use strict";

SimpleTest.waitForExplicitFinish();

var initialScreenshot;

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  var iframe = document.createElement('iframe');
  iframe.mozbrowser = true;

  
  
  
  iframe.height = '1000px';

  iframe.addEventListener('mozbrowsershowmodalprompt', function(e) {
    switch (e.detail.message) {
    case 'step 1':
      
      
      e.preventDefault();

      iframe.getScreenshot().onsuccess = function(sshot) {
        initialScreenshot = sshot.target.result;
        e.detail.unblock();
      };
      break;
    case 'step 2':
      
      
      iframe.getScreenshot().onsuccess = function(sshot) {
        is(sshot.target.result, initialScreenshot, "Screenshots should be identical");
        SimpleTest.finish();
      };
      break;
    }
  });

  document.body.appendChild(iframe);

  
  
  
  iframe.src = 'http://example.com/tests/dom/browser-element/mochitest/file_browserElement_XFrameOptionsDeny.html';
}

runTest();
