





"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

function runTest() {
  var principal = SpecialPowers.wrap(document).nodePrincipal;
  SpecialPowers.addPermission("browser", true, { url: SpecialPowers.wrap(principal.URI).spec,
                                                 appId: principal.appId,
                                                 isInBrowserElement: true });

  var iframe = document.createElement('iframe');
  iframe.setAttribute('mozbrowser', 'true');

  
  
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
                finish();
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

function finish() {
  var principal = SpecialPowers.wrap(document).nodePrincipal;
  SpecialPowers.removePermission("browser", { url: SpecialPowers.wrap(principal.URI).spec,
                                              appId: principal.appId,
                                              isInBrowserElement: true });

  SimpleTest.finish();
}

addEventListener('testready', runTest);
