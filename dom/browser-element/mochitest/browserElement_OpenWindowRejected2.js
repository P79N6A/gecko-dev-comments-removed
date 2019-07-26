









"use strict";
SimpleTest.waitForExplicitFinish();

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  var iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

  iframe.addEventListener('mozbrowseropenwindow', function(e) {
    ok(e.detail.url.indexOf('does_not_exist.html') != -1,
       'Opened URL; got ' + e.detail.url);
    is(e.detail.name, '');
    is(e.detail.features, '');

    
    
    e.preventDefault();
  });

  iframe.addEventListener('mozbrowsershowmodalprompt', function(e) {
    var msg = e.detail.message;
    if (msg.indexOf('success:') == 0) {
      ok(true, msg);
    }
    else if (msg == 'finish') {
      SimpleTest.finish();
    }
    else {
      ok(false, msg);
    }
  });

  iframe.src = 'file_browserElement_OpenWindowRejected.html';
}

runTest();
