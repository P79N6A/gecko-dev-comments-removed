









"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

var iframe;

function runTest() {
  var principal = SpecialPowers.wrap(SpecialPowers.getNodePrincipal(document));
  SpecialPowers.addPermission("browser", true, { url: SpecialPowers.wrap(principal.URI).spec,
                                                 appId: principal.appId,
                                                 isInBrowserElement: true });

  iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

  
  
  
  
  
  
  
  
  iframe.remote = false;

  iframe.addEventListener('mozbrowsershowmodalprompt', checkMessage);
  expectMessage('parent:ready', test1);

  document.body.appendChild(iframe);
  iframe.src = 'file_browserElement_SetVisibleFrames_Outer.html';
}

function test1() {
  expectMessage('child1:hidden', getVisibleTest1);
  iframe.setVisible(false);
}

function getVisibleTest1() {
  iframe.getVisible().onsuccess = function(evt) {
    ok(evt.target.result === false, 'getVisible shows a hidden frame');
    test2();
  };
}

function test2() {
  expectMessage('child1:visible', getVisibleTest2);
  iframe.setVisible(true);
}

function getVisibleTest2() {
  iframe.getVisible().onsuccess = function(evt) {
    ok(evt.target.result === true, 'getVisible shows a displayed frame');
    finish();
  };
}

function finish() {
  
  
  
  
  
  iframe.removeEventListener('mozbrowsershowmodalprompt', checkMessage);

  var principal = SpecialPowers.wrap(SpecialPowers.getNodePrincipal(document));
  SpecialPowers.removePermission("browser", { url: SpecialPowers.wrap(principal.URI).spec,
                                              appId: principal.appId,
                                              isInBrowserElement: true });

  SimpleTest.finish();
}

var expectedMsg = null;
var expectedMsgCallback = null;
function expectMessage(msg, next) {
  expectedMsg = msg;
  expectedMsgCallback = next;
}

function checkMessage(e) {
  var msg = e.detail.message;
  is(msg, expectedMsg);
  if (msg == expectedMsg) {
    expectedMsg = null;
    SimpleTest.executeSoon(function() { expectedMsgCallback() });
  }
}

addEventListener('testready', runTest);
