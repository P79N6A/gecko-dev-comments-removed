









"use strict";
SimpleTest.waitForExplicitFinish();

var iframe;
var stopped = false;
var imgSrc = 'http://test/tests/dom/browser-element/mochitest/file_bug709759.sjs';

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  iframe = document.createElement('iframe');
  iframe.mozbrowser = true;

  iframe.addEventListener('mozbrowserloadend', loadend);
  iframe.src = 'data:text/html,<html>' +
    '<body><img src="' + imgSrc + '" /></body></html>';

  document.body.appendChild(iframe);

  setTimeout(function() {
    stopped = true;
    iframe.stop();
  }, 200);
}

function loadend() {
  ok(stopped, 'Iframes network connections were stopped');
  SimpleTest.finish();
}

runTest();
