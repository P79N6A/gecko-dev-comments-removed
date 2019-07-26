

















"use strict";

SimpleTest.waitForExplicitFinish();

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  
  
  var remote = !browserElementTestHelpers.getOOPByDefaultPref();

  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;
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
      
      
      iframe.getScreenshot(1000, 1000).onsuccess = function(e) {
        var fr = FileReader();
        fr.onloadend = function() { test2(popup, fr.result); };
        fr.readAsArrayBuffer(e.target.result);
      };
    }
    else {
      ok(false, e.detail.message, "Unexpected message!");
    }
  });

  document.body.appendChild(iframe);
  iframe.src = 'file_browserElement_OpenMixedProcess.html';
}

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

function test2(popup, blankScreenshotArrayBuffer) {
  
  
  popup.getScreenshot(1000, 1000).onsuccess = function(e) {
    var fr = new FileReader();
    fr.onloadend = function() {
      if (!arrayBuffersEqual(blankScreenshotArrayBuffer, fr.result)) {
        ok(true, "Finally got a non-blank screenshot.");
        SimpleTest.finish();
        return;
      }

      SimpleTest.executeSoon(function() { test2(popup, blankScreenshot) });
    };
    fr.readAsArrayBuffer(e.target.result);
  };
}

runTest();
