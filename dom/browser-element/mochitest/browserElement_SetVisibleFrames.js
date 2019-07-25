









"use strict";

SimpleTest.waitForExplicitFinish();

var iframe;

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  iframe = document.createElement('iframe');
  iframe.mozbrowser = true;

  
  
  
  
  
  
  
  
  iframe.remote = false;

  iframe.addEventListener('mozbrowsershowmodalprompt', checkMessage);
  expectMessage('parent:ready', test1);

  document.body.appendChild(iframe);
  iframe.src = 'file_browserElement_SetVisibleFrames_Outer.html';
}

function test1() {
  expectMessage('child1:hidden', test2);
  iframe.setVisible(false);
}

function test2() {
  expectMessage('child1:visible', finish);
  iframe.setVisible(true);
}

function finish() {
  
  
  
  
  
  iframe.removeEventListener('mozbrowsershowmodalprompt', checkMessage);
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

runTest();
