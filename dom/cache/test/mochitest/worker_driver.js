



























function workerTestExec(script) {
  return new Promise(function(resolve, reject) {
    var worker = new Worker('worker_wrapper.js');
    worker.onmessage = function(event) {
      if (event.data.type == 'finish') {
        SpecialPowers.forceGC();
        resolve();

      } else if (event.data.type == 'status') {
        ok(event.data.status, event.data.msg);

      } else if (event.data.type == 'getPrefs') {
        var result = {};
        event.data.prefs.forEach(function(pref) {
          result[pref] = SpecialPowers.Services.prefs.getBoolPref(pref);
        });
        worker.postMessage({
          type: 'returnPrefs',
          prefs: event.data.prefs,
          result: result
        });

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
      }
    }

    worker.onerror = function(event) {
      reject('Worker had an error: ' + event.data);
    };

    worker.postMessage({ script: script });
  });
}
