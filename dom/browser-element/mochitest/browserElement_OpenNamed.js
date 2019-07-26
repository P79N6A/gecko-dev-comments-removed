





"use strict";
SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

var iframe;
var popupFrame;
function runTest() {
  iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

  var gotPopup = false;
  iframe.addEventListener('mozbrowseropenwindow', function(e) {
    is(gotPopup, false, 'Should get just one popup.');
    gotPopup = true;
    popupFrame = e.detail.frameElement;
    is(popupFrame.getAttribute('name'), 'OpenNamed');

    
    popupFrame.addEventListener('mozbrowsershowmodalprompt', function promptlistener(e) {
      popupFrame.removeEventListener('mozbrowsershowmodalprompt', promptlistener);

      ok(gotPopup, 'Got openwindow event before showmodalprompt event.');
      is(e.detail.message, 'success: loaded');
      SimpleTest.executeSoon(test2);
    });

    document.body.appendChild(popupFrame);
  });

  
  
  
  
  
  
  
  iframe.src = 'file_browserElement_OpenNamed.html';
  document.body.appendChild(iframe);
}

function test2() {
  popupFrame.addEventListener('mozbrowsershowmodalprompt', function(e) {
    is(e.detail.message, 'success: loaded');
    SimpleTest.finish();
  });

  iframe.src = 'file_browserElement_OpenNamed.html?test2';
}

addEventListener('testready', runTest);
