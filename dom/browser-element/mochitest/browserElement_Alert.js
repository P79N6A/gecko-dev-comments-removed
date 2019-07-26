



"use strict";

SimpleTest.waitForExplicitFinish();

var numPendingChildTests = 0;
var iframe;
var mm;

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;
  document.body.appendChild(iframe);

  mm = SpecialPowers.getBrowserFrameMessageManager(iframe);
  mm.addMessageListener('test-success', function(msg) {
    numPendingChildTests--;
    ok(true, SpecialPowers.wrap(msg).json);
  });
  mm.addMessageListener('test-fail', function(msg) {
    numPendingChildTests--;
    ok(false, SpecialPowers.wrap(msg).json);
  });

  
  
  iframe.addEventListener('mozbrowserloadend', function loadend() {
    iframe.removeEventListener('mozbrowserloadend', loadend);
    iframe.src = browserElementTestHelpers.emptyPage1;

    iframe.addEventListener('mozbrowserloadend', function loadend2() {
      iframe.removeEventListener('mozbrowserloadend', loadend2);
      SimpleTest.executeSoon(test1);
    });
  });

}

function test1() {
  iframe.addEventListener('mozbrowsershowmodalprompt', test2);

  
  
  var script = 'data:,\
    testState = 0; \
    content.alert("Hello, world!"); \
    testState = 1; \
  ';

  mm.loadFrameScript(script,  false);

  
}


function test2(e) {
  iframe.removeEventListener("mozbrowsershowmodalprompt", test2);

  is(e.detail.message, 'Hello, world!');
  e.preventDefault(); 

  SimpleTest.executeSoon(function() { test2a(e); });
}

function test2a(e) {
  
  
  var script = 'data:,\
    if (testState === 0) { \
      sendAsyncMessage("test-success", "1: Correct testState"); \
    } \
    else { \
      sendAsyncMessage("test-fail", "1: Wrong testState: " + testState); \
    }';

  mm.loadFrameScript(script,  false);
  numPendingChildTests++;

  waitForPendingTests(function() { test3(e); });
}

function test3(e) {
  
  e.detail.unblock();

  var script2 = 'data:,\
    if (testState === 1) { \
      sendAsyncMessage("test-success", "2: Correct testState"); \
    } \
    else { \
      sendAsyncMessage("test-try-again", "2: Wrong testState (for now): " + testState); \
    }';

  
  
  function onTryAgain() {
    SimpleTest.executeSoon(function() {
      
      mm.loadFrameScript(script2,  false);
    });
  }

  mm.addMessageListener('test-try-again', onTryAgain);
  numPendingChildTests++;

  onTryAgain();
  waitForPendingTests(function() { test4(); });
}

function test4() {
  
  

  iframe.addEventListener("mozbrowsershowmodalprompt", test5);

  var script = 'data:,content.alert("test4");';
  mm.loadFrameScript(script,  false);
}


function test5(e) {
  iframe.removeEventListener('mozbrowsershowmodalprompt', test4);

  is(e.detail.message, 'test4');
  e.preventDefault(); 

  SimpleTest.executeSoon(test5a);
}

function test5a() {
  iframe.addEventListener('mozbrowserloadend', test5b);
  iframe.src = browserElementTestHelpers.emptyPage2;
}

function test5b() {
  iframe.removeEventListener('mozbrowserloadend', test5b);
  SimpleTest.finish();
}

var prevNumPendingTests = null;
function waitForPendingTests(next) {
  if (numPendingChildTests !== prevNumPendingTests) {
    dump("Waiting for end; " + numPendingChildTests + " pending tests\n");
    prevNumPendingTests = numPendingChildTests;
  }

  if (numPendingChildTests > 0) {
    SimpleTest.executeSoon(function() { waitForPendingTests(next); });
    return;
  }

  prevNumPendingTests = null;
  next();
}

runTest();

