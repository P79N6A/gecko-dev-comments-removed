


function serviceWorkerTestExec(testFile) {
  var isB2G = !navigator.userAgent.contains("Android") &&
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
        worker.postMessage({ script: testFile });
      };
      document.body.appendChild(iframe);
    }

    navigator.serviceWorker.register("worker_wrapper.js" + "?" + (Math.random()), {scope: "."})
      .then(function(registration) {
        if (registration.installing) {
          registration.installing.onstatechange = function(e) {
            e.target.onstatechange = null;
            setupSW(registration);
          };
        } else {
          setupSW(registration);
        }
      });
  });
}
