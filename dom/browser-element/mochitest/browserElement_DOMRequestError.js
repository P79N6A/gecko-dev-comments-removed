




"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function runTest() {
  var iframe1 = document.createElement('iframe');
  SpecialPowers.wrap(iframe1).mozbrowser = true;
  iframe1.src = 'data:text/html,<html>' +
    '<body style="background:green">hello</body></html>';
  document.body.appendChild(iframe1);

  function testIframe(beforeRun, isErrorExpected, nextTest) {
    return function() {
      var error = false;
      if (beforeRun)
        beforeRun();
      function testEnd() {
        is(isErrorExpected, error);
        SimpleTest.executeSoon(nextTest);
      }

      var domRequest = iframe1.getScreenshot(1000, 1000);
      domRequest.onsuccess = function(e) {
        testEnd();
      }
      domRequest.onerror = function(e) {
        error = true;
        testEnd();
      }
    };
  }

  function iframeLoadedHandler() {
    iframe1.removeEventListener('mozbrowserloadend', iframeLoadedHandler);
    
    
    
    var test3 = testIframe(
      function() {
        document.body.appendChild(iframe1);
      }, false,
      function() {
        SimpleTest.finish();
      })
    ;
    var test2 = testIframe(function() {
      document.body.removeChild(iframe1);
    }, true, test3);
    var test1 = testIframe(null, false, test2);
    SimpleTest.executeSoon(test1);
  }

  iframe1.addEventListener('mozbrowserloadend', iframeLoadedHandler);
}

addEventListener('testready', runTest);
