



























function workerTestExec(script) {
  SimpleTest.waitForExplicitFinish();
  var worker = new Worker('worker_wrapper.js');
  worker.onmessage = function(event) {
    if (event.data.type == 'finish') {
      SimpleTest.finish();

    } else if (event.data.type == 'status') {
      ok(event.data.status, event.data.msg);

    } else if (event.data.type == 'getPermissions') {
      var result = {};
      event.data.permissions.forEach(function(permission) {
        result[permission] = SpecialPowers.hasPermission(permission, window.document);
      });
      worker.postMessage({
        type: 'returnPermissions',
        permissions: event.data.permissions,
        result: result
      });

    } else if (event.data.type == 'getVersion') {
      var result = SpecialPowers.Cc['@mozilla.org/xre/app-info;1'].getService(SpecialPowers.Ci.nsIXULAppInfo).version;
      worker.postMessage({
        type: 'returnVersion',
        result: result
      });

    } else if (event.data.type == 'getUserAgent') {
      worker.postMessage({
        type: 'returnUserAgent',
        result: navigator.userAgent
      });
    } else if (event.data.type == 'getOSCPU') {
      worker.postMessage({
        type: 'returnOSCPU',
        result: navigator.oscpu
      });
    } else if (event.data.type == 'getIsB2G') {
      worker.postMessage({
        type: 'returnIsB2G',
        result: SpecialPowers.isB2G
      });
    }
  }

  worker.onerror = function(event) {
    ok(false, 'Worker had an error: ' + event.data);
    SimpleTest.finish();
  };

  worker.postMessage({ script: script });
}
