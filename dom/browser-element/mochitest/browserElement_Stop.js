









"use strict";
SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

var iframe;
var stopped = false;
var imgSrc = 'http://test/tests/dom/browser-element/mochitest/file_bug709759.sjs';

function runTest() {
  iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;

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

  
  iframe.addEventListener('mozbrowsererror', handleError);
  window.setTimeout(function() {
    iframe.removeEventListener('mozbrowsererror', handleError);
    SimpleTest.finish();
  }, 1000);
}

function handleError() {
  ok(false, "mozbrowsererror should not be fired");
}

addEventListener('testready', runTest);
