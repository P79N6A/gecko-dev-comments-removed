




"use strict";
SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function runTest() {
  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

  iframe.addEventListener('mozbrowseropenwindow', function(e) {
    ok(true, 'Got first window.open call');

    e.detail.frameElement.addEventListener('mozbrowseropenwindow', function(e) {
      ok(true, 'Got second window.open call');
      document.body.appendChild(e.detail.frameElement);
    });

    e.detail.frameElement.addEventListener('mozbrowsershowmodalprompt', function(e) {
      ok(true, 'Got alert from second window.');
      SimpleTest.finish();
    });

    document.body.appendChild(e.detail.frameElement);
  });

  
  
  
  
  

  iframe.src = 'http://example.org/tests/dom/browser-element/mochitest/file_browserElement_OpenWindowDifferentOrigin.html?1';
  document.body.appendChild(iframe);
}

addEventListener('testready', runTest);
