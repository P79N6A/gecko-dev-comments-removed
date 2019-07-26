






"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function runTest() {
  var iframe1 = document.createElement('iframe');
  SpecialPowers.wrap(iframe1).mozbrowser = true;

  
  
  

  iframe1.addEventListener('mozbrowseropenwindow', function(e) {
    ok(true, "Got first mozbrowseropenwindow event.");
    document.body.appendChild(e.detail.frameElement);

    e.detail.frameElement.addEventListener('mozbrowserlocationchange', function(e) {
      if (e.detail == "http://example.com/#2") {
        ok(true, "Got locationchange to http://example.com/#2");
        SimpleTest.finish();
      }
      else {
        ok(true, "Got locationchange to " + e.detail);
      }
    });

    SimpleTest.executeSoon(function() {
      var iframe2 = document.createElement('iframe');
      SpecialPowers.wrap(iframe2).mozbrowser = true;

      iframe2.addEventListener('mozbrowseropenwindow', function(e) {
        ok(false, "Got second mozbrowseropenwindow event.");
      });

      document.body.appendChild(iframe2);
      iframe2.src = 'file_browserElement_BrowserWindowNamespace.html#2';
    });
  });

  document.body.appendChild(iframe1);
  iframe1.src = 'file_browserElement_BrowserWindowNamespace.html#1';
}

addEventListener('testready', runTest);
