"use strict";

SimpleTest.waitForExplicitFinish();

var iframe;
var loadedEvents = 0;

function loadServer() {
  var url = SimpleTest.getTestFileURL("file_loadserver.js");
  var script = SpecialPowers.loadChromeScript(url);
}

function runTest() {
  iframe = document.createElement('iframe');
  document.body.appendChild(iframe);
  iframe.addEventListener('load', mozbrowserLoaded);
  iframe.src = 'file_bug1110872.html';
}

function iframeBodyRecv(msg) {
  switch (loadedEvents) {
  case 1:
    
    
    ok(true, 'got response from first test!');
    break;
  case 2:
    
    
    
    
    ok(true, 'further queries returned ok after SettingsManager death');
    SimpleTest.finish();
    break;
  }
}

function mozbrowserLoaded() {
  loadedEvents++;
  iframe.contentWindow.postMessage({name: "start", step: loadedEvents}, '*');
  window.addEventListener('message', iframeBodyRecv);
}

window.addEventListener("load", function() {
  loadServer();
  runTest();
});
