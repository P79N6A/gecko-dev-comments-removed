



'use strict';

const kInterfaceName = 'wifi';

var server;
var step = 0;
var loginFinished = false;

function xhr_handler(metadata, response) {
  response.setStatusLine(metadata.httpVersion, 200, 'OK');
  response.setHeader('Cache-Control', 'no-cache', false);
  response.setHeader('Content-Type', 'text/plain', false);
  if (loginFinished) {
    response.write('true');
  } else {
    response.write('false');
  }
}

function fakeUIResponse() {
  let requestId;
  Services.obs.addObserver(function observe(subject, topic, data) {
    if (topic === 'captive-portal-login') {
      let xhr = Cc['@mozilla.org/xmlextras/xmlhttprequest;1']
                  .createInstance(Ci.nsIXMLHttpRequest);
      xhr.open('GET', gServerURL + kCanonicalSitePath, true);
      xhr.send();
      loginFinished = true;
      do_check_eq(++step, 2);
      requestId = JSON.parse(data).id;
      gCaptivePortalDetector.abort(kInterfaceName);
    }
  }, 'captive-portal-login', false);
  Services.obs.addObserver(function observe(subject, topic, data) {
    if (topic === 'captive-portal-login-abort') {
      do_check_eq(++step, 3);
      do_check_eq(JSON.parse(data).id, requestId);
      gServer.stop(do_test_finished);
    }
  }, 'captive-portal-login-abort', false);
}

function test_abort() {
  do_test_pending();

  let callback = {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsICaptivePortalCallback]),
    prepare: function prepare() {
      do_check_eq(++step, 1);
      gCaptivePortalDetector.finishPreparation(kInterfaceName);
    },
    complete: function complete(success) {
      do_throw('should not execute |complete| callback');
    },
  };

  gCaptivePortalDetector.checkCaptivePortal(kInterfaceName, callback);
}

function run_test() {
  run_captivedetect_test(xhr_handler, fakeUIResponse, test_abort);
}
