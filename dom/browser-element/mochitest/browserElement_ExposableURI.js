





"use strict";
SimpleTest.waitForExplicitFinish();

var iframe;


function testPassword() {
  function locationchange(e) {
    var uri = e.detail;
    is(uri, 'http://mochi.test:8888/tests/dom/browser-element/mochitest/file_empty.html',
       "Username and password shouldn't be exposed in uri.");
    SimpleTest.finish();
  }

  iframe.addEventListener('mozbrowserlocationchange', locationchange);
  iframe.src = "http://iamuser:iampassword@mochi.test:8888/tests/dom/browser-element/mochitest/file_empty.html";
}

function testWyciwyg() {
  var locationChangeCount = 0;

  function locationchange(e) {
    
    
    
    if (locationChangeCount == 0) {
      locationChangeCount ++;
    } else if (locationChangeCount == 1) {
      var uri = e.detail;
      is(uri, 'http://mochi.test:8888/tests/dom/browser-element/mochitest/file_wyciwyg.html', "Scheme in string shouldn't be wyciwyg");
      iframe.removeEventListener('mozbrowserlocationchange', locationchange);
      SimpleTest.executeSoon(testPassword);
    }
  }

  
  iframe.src = 'file_wyciwyg.html';
  iframe.addEventListener('mozbrowserlocationchange', locationchange);
}

function runTest() {
  browserElementTestHelpers.setEnabledPref(true);
  browserElementTestHelpers.addPermission();

  iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;
  document.body.appendChild(iframe);
  testWyciwyg();
}

addEventListener('load', function() { SimpleTest.executeSoon(runTest); });
