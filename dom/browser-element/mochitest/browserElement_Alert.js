



"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

var numPendingChildTests = 0;
var iframe;
var mm;

function runTest() {
  iframe = document.createElement('iframe');
  iframe.setAttribute('mozbrowser', 'true');
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
    this.testState = 0; \
    content.alert("Hello, world!"); \
    this.testState = 1; \
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
    if (this.testState === 0) { \
      sendAsyncMessage("test-success", "1: Correct testState"); \
    } \
    else { \
      sendAsyncMessage("test-fail", "1: Wrong testState: " + this.testState); \
    }';

  mm.loadFrameScript(script,  false);
  numPendingChildTests++;

  waitForPendingTests(function() { test3(e); });
}

function test3(e) {
  
  e.detail.unblock();

  var script2 = 'data:,\
    if (this.testState === 1) { \
      sendAsyncMessage("test-success", "2: Correct testState"); \
    } \
    else { \
      sendAsyncMessage("test-try-again", "2: Wrong testState (for now): " + this.testState); \
    }';

  
  
  function onTryAgain() {
    SimpleTest.executeSoon(function() {
      
      mm.loadFrameScript(script2,  false);
    });
  }

  mm.addMessageListener('test-try-again', onTryAgain);
  numPendingChildTests++;

  onTryAgain();
  waitForPendingTests(function() {
    mm.removeMessageListener('test-try-again', onTryAgain);
    test4();
  });
}

function test4() {
  
  

  iframe.addEventListener("mozbrowsershowmodalprompt", test5);

  var script = 'data:,content.alert("test4");';
  mm.loadFrameScript(script,  false);
}


function test5(e) {
  iframe.removeEventListener('mozbrowsershowmodalprompt', test5);

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
  SimpleTest.executeSoon(test6);
}


var promptBlockers = [];
function test6() {
  iframe.addEventListener("mozbrowsershowmodalprompt", test6a);

  var script = 'data:,\
    this.testState = 0; \
    content.alert(1); \
    this.testState = 3; \
  ';
  mm.loadFrameScript(script,  false);
}

function test6a(e) {
  iframe.removeEventListener("mozbrowsershowmodalprompt", test6a);

  is(e.detail.message, '1');
  e.preventDefault(); 
  promptBlockers.push(e);

  SimpleTest.executeSoon(test6b);
}

function test6b() {
  var script = 'data:,\
    if (this.testState === 0) { \
      sendAsyncMessage("test-success", "1: Correct testState"); \
    } \
    else { \
      sendAsyncMessage("test-fail", "1: Wrong testState: " + this.testState); \
    }';
  mm.loadFrameScript(script,  false);
  numPendingChildTests++;

  waitForPendingTests(test6c);
}

function test6c() {
  iframe.addEventListener("mozbrowsershowmodalprompt", test6d);

  var script = 'data:,\
    this.testState = 1; \
    content.alert(2); \
    this.testState = 2; \
  ';
  mm.loadFrameScript(script,  false);
}

function test6d(e) {
  iframe.removeEventListener("mozbrowsershowmodalprompt", test6d);

  is(e.detail.message, '2');
  e.preventDefault(); 
  promptBlockers.push(e);

  SimpleTest.executeSoon(test6e);
}

function test6e() {
  var script = 'data:,\
    if (this.testState === 1) { \
      sendAsyncMessage("test-success", "2: Correct testState"); \
    } \
    else { \
      sendAsyncMessage("test-fail", "2: Wrong testState: " + this.testState); \
    }';
  mm.loadFrameScript(script,  false);
  numPendingChildTests++;

  waitForPendingTests(test6f);
}

function test6f() {
  var e = promptBlockers.pop();
  
  e.detail.unblock();

  var script2 = 'data:,\
    if (this.testState === 2) { \
      sendAsyncMessage("test-success", "3: Correct testState"); \
    } \
    else { \
      sendAsyncMessage("test-try-again", "3: Wrong testState (for now): " + this.testState); \
    }';

  
  
  function onTryAgain() {
    SimpleTest.executeSoon(function() {
      
      mm.loadFrameScript(script2,  false);
    });
  }

  mm.addMessageListener('test-try-again', onTryAgain);
  numPendingChildTests++;

  onTryAgain();
  waitForPendingTests(function() {
    mm.removeMessageListener('test-try-again', onTryAgain);
    test6g();
  });
}

function test6g() {
  var e = promptBlockers.pop();
  
  e.detail.unblock();

  var script2 = 'data:,\
    if (this.testState === 3) { \
      sendAsyncMessage("test-success", "4: Correct testState"); \
    } \
    else { \
      sendAsyncMessage("test-try-again", "4: Wrong testState (for now): " + this.testState); \
    }';

  
  
  function onTryAgain() {
    SimpleTest.executeSoon(function() {
      
      mm.loadFrameScript(script2,  false);
    });
  }

  mm.addMessageListener('test-try-again', onTryAgain);
  numPendingChildTests++;

  onTryAgain();
  waitForPendingTests(function() {
    mm.removeMessageListener('test-try-again', onTryAgain);
    test6h();
  });
}

function test6h() {
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

addEventListener('testready', runTest);
