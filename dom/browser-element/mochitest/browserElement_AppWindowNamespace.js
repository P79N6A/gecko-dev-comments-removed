



"use strict";

SimpleTest.waitForExplicitFinish();

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();
  
  SpecialPowers.addPermission("embed-apps", true, document);

  var iframe1 = document.createElement('iframe');
  SpecialPowers.wrap(iframe1).mozbrowser = true;
  iframe1.setAttribute('mozapp', 'http://example.org/manifest.webapp');

  
  
  

  iframe1.addEventListener('mozbrowseropenwindow', function(e) {
    ok(true, "Got first mozbrowseropenwindow event.");
    document.body.appendChild(e.detail.frameElement);

    SimpleTest.executeSoon(function() {
      var iframe2 = document.createElement('iframe');
      SpecialPowers.wrap(iframe2).mozbrowser = true;
      iframe2.setAttribute('mozapp', 'http://example.com/manifest.webapp');

      iframe2.addEventListener('mozbrowseropenwindow', function(e) {
        ok(true, "Got second mozbrowseropenwindow event.");
        SpecialPowers.removePermission("embed-apps", document);
        SimpleTest.finish();
      });

      document.body.appendChild(iframe2);
      iframe2.src = 'http://example.com/tests/dom/browser-element/mochitest/file_browserElement_AppWindowNamespace.html';
    });
  });

  document.body.appendChild(iframe1);
  iframe1.src = 'http://example.org/tests/dom/browser-element/mochitest/file_browserElement_AppWindowNamespace.html';
}

runTest();
