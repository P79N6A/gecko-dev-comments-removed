function testScript(script) {
  function setupPrefs() {
    return new Promise(function(resolve, reject) {
      SpecialPowers.pushPrefEnv({
        "set": [["dom.serviceWorkers.enabled", true],
                ["dom.serviceWorkers.testing.enabled", true],
                ["dom.serviceWorkers.exemptFromPerDomainMax", true]]
      }, resolve);
    });
  }

  function workerTest() {
    return new Promise(function(resolve, reject) {
      var worker = new Worker("worker_wrapper.js");
      worker.onmessage = function(event) {
        if (event.data.context != "Worker") {
          return;
        }
        if (event.data.type == 'finish') {
          resolve();
        } else if (event.data.type == 'status') {
          ok(event.data.status, event.data.context + ": " + event.data.msg);
        }
      }
      worker.onerror = function(event) {
        reject("Worker error: " + event.message);
      };

      worker.postMessage({ "script": script });
    });
  }

  function serviceWorkerTest() {
    var isB2G = !navigator.userAgent.includes("Android") &&
                /Mobile|Tablet/.test(navigator.userAgent);
    if (isB2G) {
      
      dump("Skipping running the test in SW until bug 1137683 gets fixed.\n");
      return Promise.resolve();
    }
    return new Promise(function(resolve, reject) {
      function setupSW(registration) {
        var worker = registration.waiting ||
                     registration.active;

        window.addEventListener("message",function onMessage(event) {
          if (event.data.context != "ServiceWorker") {
            return;
          }
          if (event.data.type == 'finish') {
            window.removeEventListener("message", onMessage);
            registration.unregister()
              .then(resolve)
              .catch(reject);
          } else if (event.data.type == 'status') {
            ok(event.data.status, event.data.context + ": " + event.data.msg);
          }
        }, false);

        worker.onerror = reject;

        var iframe = document.createElement("iframe");
        iframe.src = "message_receiver.html";
        iframe.onload = function() {
          worker.postMessage({ script: script });
        };
        document.body.appendChild(iframe);
      }

      navigator.serviceWorker.ready.then(setupSW);
      navigator.serviceWorker.register("worker_wrapper.js", {scope: "."});
    });
  }

  function windowTest() {
    return new Promise(function(resolve, reject) {
      var scriptEl = document.createElement("script");
      scriptEl.setAttribute("src", script);
      scriptEl.onload = function() {
        runTest().then(resolve, reject);
      };
      document.body.appendChild(scriptEl);
    });
  }

  SimpleTest.waitForExplicitFinish();
  
  
  
  setupPrefs()
    .then(function() {
      return windowTest();
    })
    .then(function() {
      return workerTest();
    })
    .then(function() {
      return serviceWorkerTest();
    })
    .catch(function(e) {
      ok(false, "Some test failed in " + script);
      info(e);
      info(e.message);
      return Promise.resolve();
    })
    .then(function() {
      SimpleTest.finish();
    });
}

