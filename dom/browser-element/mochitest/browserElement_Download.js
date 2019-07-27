




"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.addPermission();

var iframe;
var downloadURL = 'http://test/tests/dom/browser-element/mochitest/file_download_bin.sjs';

function runTest() {
  iframe = document.createElement('iframe');
  iframe.setAttribute('mozbrowser', 'true');

  iframe.addEventListener('mozbrowserloadend', loadend);
  iframe.src = 'data:text/html,<html><body>hello</body></html>';
  iframe.setAttribute('remote', 'true');

  document.body.appendChild(iframe);
}

function loadend() {
  var req = iframe.download(downloadURL, { filename: 'test.bin' });
  req.onsuccess = function() {
    ok(true, 'Download finished as expected.');
    SimpleTest.finish();
  }
  req.onerror = function() {
    ok(false, 'Expected no error, got ' + req.error);
  }
}

addEventListener('testready', runTest);
