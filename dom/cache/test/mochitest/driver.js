












function runTests(testFile) {
  function setupPrefs() {
    return new Promise(function(resolve, reject) {
      SpecialPowers.pushPrefEnv({
        "set": [["dom.caches.enabled", true],
                ["dom.serviceWorkers.enabled", true],
                ["dom.serviceWorkers.testing.enabled", true],
                ["dom.serviceWorkers.exemptFromPerDomainMax", true]]
      }, function() {
        resolve();
      });
    });
  }

  function loadScript(script) {
    return new Promise(function(resolve, reject) {
      var s = document.createElement("script");
      s.src = script;
      s.onerror = reject;
      s.onload = resolve;
      document.body.appendChild(s);
    });
  }

  function importDrivers() {
    return Promise.all([loadScript("worker_driver.js"),
                        loadScript("serviceworker_driver.js")]);
  }

  function runWorkerTest() {
    return workerTestExec(testFile);
  }

  function runServiceWorkerTest() {
    return serviceWorkerTestExec(testFile);
  }

  function runFrameTest() {
    return new Promise(function(resolve, reject) {
      var iframe = document.createElement("iframe");
      iframe.src = "frame.html";
      iframe.onload = function() {
        var doc = iframe.contentDocument;
        var s = doc.createElement("script");
        s.src = testFile;
        window.onmessage = function(event) {
          if (event.data.type == 'finish') {
            window.onmessage = null;
            resolve();
          } else if (event.data.type == 'status') {
            ok(event.data.status, event.data.msg);
          }
        };
        doc.body.appendChild(s);
      };
      document.body.appendChild(iframe);
    });
  }

  SimpleTest.waitForExplicitFinish();
  return setupPrefs()
      .then(importDrivers)
      
      
      
      .then(runWorkerTest)
      .then(runServiceWorkerTest)
      .then(runFrameTest)
      .catch(function(e) {
        ok(false, "A promise was rejected during test execution: " + e);
      });
}

