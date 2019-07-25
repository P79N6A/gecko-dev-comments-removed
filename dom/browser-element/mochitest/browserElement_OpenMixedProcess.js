

















"use strict";

SimpleTest.waitForExplicitFinish();

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  
  
  var remote = !browserElementTestHelpers.getOOPByDefaultPref();

  var iframe = document.createElement('iframe');
  iframe.mozbrowser = true;
  iframe.setAttribute('remote', remote);

  
  
  
  
  
  
  
  
  var popup;
  iframe.addEventListener('mozbrowseropenwindow', function(e) {
    popup = document.body.appendChild(e.detail.frameElement);
  });

  iframe.addEventListener('mozbrowsershowmodalprompt', function(e) {
    if (e.detail.message.startsWith('pass')) {
      ok(true, e.detail.message);
    }
    else if (e.detail.message.startsWith('fail')) {
      ok(false, e.detail.message);
    }
    else if (e.detail.message == 'finish') {
      
      
      iframe.getScreenshot().onsuccess = function(e) {
        test2(popup, e.target.result, popup);
      };
    }
    else {
      ok(false, e.detail.message, "Unexpected message!");
    }
  });

  document.body.appendChild(iframe);
  iframe.src = 'file_browserElement_OpenMixedProcess.html';
}

var prevScreenshot;
function test2(popup, blankScreenshot) {
  
  
  popup.getScreenshot().onsuccess = function(e) {
    var screenshot = e.target.result;
    if (screenshot != blankScreenshot) {
      SimpleTest.finish();
      return;
    }

    if (screenshot != prevScreenshot) {
      prevScreenshot = screenshot;
      dump("Got screenshot: " + screenshot + "\n");
    }
    SimpleTest.executeSoon(function() { test2(popup, blankScreenshot) });
  };
}

runTest();
