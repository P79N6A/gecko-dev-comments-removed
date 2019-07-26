



'use strict';

const kInterfaceName = 'wifi';
const kOtherInterfaceName = 'ril';

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
  Services.obs.addObserver(function observe(subject, topic, data) {
    if (topic === 'captive-portal-login') {
      let xhr = Cc['@mozilla.org/xmlextras/xmlhttprequest;1']
                  .createInstance(Ci.nsIXMLHttpRequest);
      xhr.open('GET', kServerURL + kCanonicalSitePath, true);
      xhr.send();
      loginFinished = true;
      do_check_eq(++step, 2);
    }
  }, 'captive-portal-login', false);
}

function test_multiple_requests() {
  do_test_pending();

  let callback = {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsICaptivePortalCallback]),
    prepare: function prepare() {
      do_check_eq(++step, 1);
      gCaptivePortalDetector.finishPreparation(kInterfaceName);
    },
    complete: function complete(success) {
      do_check_eq(++step, 3);
      do_check_true(success);
    },
  };

  let otherCallback = {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsICaptivePortalCallback]),
    prepare: function prepare() {
      do_check_eq(++step, 4);
      gCaptivePortalDetector.finishPreparation(kOtherInterfaceName);
    },
    complete: function complete(success) {
      do_check_eq(++step, 5);
      do_check_true(success);
      gServer.stop(do_test_finished);
    }
  };

  gCaptivePortalDetector.checkCaptivePortal(kInterfaceName, callback);
  gCaptivePortalDetector.checkCaptivePortal(kOtherInterfaceName, otherCallback);
}

function run_test() {
  run_captivedetect_test(xhr_handler, fakeUIResponse, test_multiple_requests);
}
