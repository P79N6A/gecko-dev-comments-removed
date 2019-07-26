



'use strict';

const kInterfaceName = 'wifi';

var server;
var step = 0;
var attempt = 0;

function xhr_handler(metadata, response) {
  dump('HTTP activity\n');
  response.setStatusLine(metadata.httpVersion, 200, 'OK');
  response.setHeader('Cache-Control', 'no-cache', false);
  response.setHeader('Content-Type', 'text/plain', false);
  response.write('true');
  attempt++;
}

function fakeUIResponse() {
  Services.obs.addObserver(function observe(subject, topic, data) {
    if (topic == 'captive-portal-login') {
      do_throw('should not receive captive-portal-login event');
    }
  }, 'captive-portal-login', false);
}

function test_portal_not_found() {
  do_test_pending();

  let callback = {
    QueryInterface: XPCOMUtils.generateQI([Ci.nsICaptivePortalCallback]),
    prepare: function prepare() {
      do_check_eq(++step, 1);
      gCaptivePortalDetector.finishPreparation(kInterfaceName);
    },
    complete: function complete(success) {
      do_check_eq(++step, 2);
      do_check_true(success);
      do_check_eq(attempt, 1);
      server.stop(function(){dump('server stop\n'); do_test_finished(); });
    }
  };

  gCaptivePortalDetector.checkCaptivePortal(kInterfaceName, callback);
}

function run_test() {
  server = new HttpServer();
  server.registerPathHandler(kCanonicalSitePath, xhr_handler);
  server.start(4444);

  fakeUIResponse();

  test_portal_not_found();
}
