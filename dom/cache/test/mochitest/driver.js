

















function runTests(testFile, order) {
  function setupPrefs() {
    return new Promise(function(resolve, reject) {
      SpecialPowers.pushPrefEnv({
        "set": [["dom.caches.enabled", true],
                ["dom.caches.testing.enabled", true],
                ["dom.serviceWorkers.enabled", true],
                ["dom.serviceWorkers.testing.enabled", true],
                ["dom.serviceWorkers.exemptFromPerDomainMax", true]]
      }, function() {
        resolve();
      });
    });
  }

  
  function clearStorage() {
    return new Promise(function(resolve, reject) {
      var principal = SpecialPowers.wrap(document).nodePrincipal;
      var appId, inBrowser;
      var nsIPrincipal = SpecialPowers.Components.interfaces.nsIPrincipal;
      if (principal.appId != nsIPrincipal.UNKNOWN_APP_ID &&
          principal.appId != nsIPrincipal.NO_APP_ID) {
        appId = principal.appId;
        inBrowser = principal.isInBrowserElement;
      }
      SpecialPowers.clearStorageForURI(document.documentURI, resolve, appId,
                                       inBrowser);
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
        window.addEventListener("message", function onMessage(event) {
          if (event.data.context != "Window") {
            return;
          }
          if (event.data.type == 'finish') {
            window.removeEventListener("message", onMessage);
            resolve();
          } else if (event.data.type == 'status') {
            ok(event.data.status, event.data.context + ": " + event.data.msg);
          }
        }, false);
        doc.body.appendChild(s);
      };
      document.body.appendChild(iframe);
    });
  }

  SimpleTest.waitForExplicitFinish();

  if (typeof order == "undefined") {
    order = "sequential"; 
    
  }

  ok(order == "parallel" || order == "sequential" || order == "both",
     "order argument should be valid");

  if (order == "both") {
    info("Running tests in both modes; first: sequential");
    return runTests(testFile, "sequential")
        .then(function() {
          info("Running tests in parallel mode");
          return runTests(testFile, "parallel");
        });
  }
  if (order == "sequential") {
    return setupPrefs()
        .then(importDrivers)
        .then(runWorkerTest)
        .then(clearStorage)
        .then(runServiceWorkerTest)
        .then(clearStorage)
        .then(runFrameTest)
        .then(clearStorage)
        .catch(function(e) {
          ok(false, "A promise was rejected during test execution: " + e);
        });
  }
  return setupPrefs()
      .then(importDrivers)
      .then(() => Promise.all([runWorkerTest(), runServiceWorkerTest(), runFrameTest()]))
      .then(clearStorage)
      .catch(function(e) {
        ok(false, "A promise was rejected during test execution: " + e);
      });
}

