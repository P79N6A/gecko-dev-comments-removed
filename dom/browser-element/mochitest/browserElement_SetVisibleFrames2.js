





"use strict";

SimpleTest.waitForExplicitFinish();

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  var iframe = document.createElement('iframe');
  iframe.mozbrowser = true;

  
  
  iframe.remote = false;

  iframe.addEventListener('mozbrowserloadend', function loadEnd(e) {
    iframe.removeEventListener('mozbrowserloadend', loadEnd);
    iframe.setVisible(false);
    iframe.src = 'file_browserElement_SetVisibleFrames2_Outer.html';
  });

  iframe.addEventListener('mozbrowsershowmodalprompt', function(e) {
    if (e.detail.message == 'parent:finish') {
      ok(true, "Got parent:finish");

      
      SimpleTest.executeSoon(function() {
        SimpleTest.executeSoon(function() {
          SimpleTest.executeSoon(function() {
            SimpleTest.executeSoon(function() {
              SimpleTest.executeSoon(function() {
                SimpleTest.finish();
              });
            });
          });
        });
      });
    }
    else {
      ok(false, "Got unexpected message: " + e.detail.message);
    }
  });

  document.body.appendChild(iframe);
}

runTest();
