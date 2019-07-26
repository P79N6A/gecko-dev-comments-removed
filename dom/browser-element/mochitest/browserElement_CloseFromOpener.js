



"use strict";

SimpleTest.waitForExplicitFinish();

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

  iframe.addEventListener('mozbrowseropenwindow', function(e) {
    ok(true, "got openwindow event.");
    document.body.appendChild(e.detail.frameElement);

    e.detail.frameElement.addEventListener("mozbrowserclose", function(e) {
      ok(true, "got mozbrowserclose event.");
      SimpleTest.finish();
    });
  });


  document.body.appendChild(iframe);

  
  
  iframe.src = "file_browserElement_CloseFromOpener.html";
}

runTest();
