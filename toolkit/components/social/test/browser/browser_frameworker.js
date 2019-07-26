






function makeWorkerUrl(runner) {
  let prefix =  "http://example.com/browser/toolkit/components/social/test/browser/echo.sjs?";
  if (typeof runner == "function") {
    runner = "var run=" + runner.toSource() + ";run();";
  }
  return prefix + encodeURI(runner);
}

var getFrameWorkerHandle;
function test() {
  waitForExplicitFinish();

  let scope = {};
  Cu.import("resource://gre/modules/FrameWorker.jsm", scope);
  getFrameWorkerHandle = scope.getFrameWorkerHandle;

  runTests(tests);
}

let tests = {
  testSimple: function(cbnext) {
    let run = function() {
      onconnect = function(e) {
        let port = e.ports[0];
        port.onmessage = function(e) {
          if (e.data.topic == "ping") {
            port.postMessage({topic: "pong"});
          }
        }
      }
    }

    let worker = getFrameWorkerHandle(makeWorkerUrl(run), undefined, "testSimple");

    worker.port.onmessage = function(e) {
      if (e.data.topic == "pong") {
        worker.terminate();
        cbnext();
      }
    }
    worker.port.postMessage({topic: "ping"})
  },

  
  testEarlyClose: function(cbnext) {
    let run = function() {
      onconnect = function(e) {
        let port = e.ports[0];
        port.postMessage({topic: "oh hai"});
      }
    }

    let worker = getFrameWorkerHandle(makeWorkerUrl(run), undefined, "testEarlyClose");
    worker.port.close();
    worker.terminate();
    cbnext();
  },

  
  testPortClosingMessage: function(cbnext) {
    
    let run = function() {
      let firstPort, secondPort;
      onconnect = function(e) {
        let port = e.ports[0];
        if (firstPort === undefined) {
          firstPort = port;
          port.onmessage = function(e) {
            if (e.data.topic == "social.port-closing") {
              secondPort.postMessage({topic: "got-closing"});
            }
          }
        } else {
          secondPort = port;
          
          
          secondPort.postMessage({topic: "connected"});
        }
      }
    }
    let workerurl = makeWorkerUrl(run);
    let worker1 = getFrameWorkerHandle(workerurl, undefined, "testPortClosingMessage worker1");
    let worker2 = getFrameWorkerHandle(workerurl, undefined, "testPortClosingMessage worker2");
    worker2.port.onmessage = function(e) {
      if (e.data.topic == "connected") {
        
        worker1.port.close();
      } else if (e.data.topic == "got-closing") {
        worker2.terminate();
        cbnext();
      }
    }
  },

  
  
  testPrototypes: function(cbnext) {
    let run = function() {
      
      Array.prototype.customfunction = function() {};
      onconnect = function(e) {
        let port = e.ports[0];
        port.onmessage = function(e) {
          
          if (e.data.topic == "hello" && e.data.data.customfunction) {
            port.postMessage({topic: "hello", data: [1,2,3]});
          }
        }
      }
    }
    
    
    
    
    let fakeWindow = {
      JSON: {
        parse: function(s) {
          let data = JSON.parse(s);
          data.data.somextrafunction = function() {};
          return data;
        }
      }
    }
    let worker = getFrameWorkerHandle(makeWorkerUrl(run), fakeWindow, "testPrototypes");
    worker.port.onmessage = function(e) {
      if (e.data.topic == "hello") {
        ok(e.data.data.somextrafunction, "have someextrafunction")
        worker.terminate();
        cbnext();
      }
    }
    worker.port.postMessage({topic: "hello", data: [1,2,3]});
  },

  testSameOriginImport: function(cbnext) {
    let run = function() {
      onconnect = function(e) {
        let port = e.ports[0];
        port.onmessage = function(e) {
          if (e.data.topic == "ping") {
            try {
              importScripts("http://mochi.test:8888/error");
            } catch(ex) {
              port.postMessage({topic: "pong", data: ex});
              return;
            }
            port.postMessage({topic: "pong", data: null});
          }
        }
      }
    }

    let worker = getFrameWorkerHandle(makeWorkerUrl(run), undefined, "testSameOriginImport");
    worker.port.onmessage = function(e) {
      if (e.data.topic == "pong") {
        isnot(e.data.data, null, "check same-origin applied to importScripts");
        worker.terminate();
        cbnext();
      }
    }
    worker.port.postMessage({topic: "ping"})
  },

  testRelativeImport: function(cbnext) {
    let url = "https://example.com/browser/toolkit/components/social/test/browser/worker_relative.js";
    let worker = getFrameWorkerHandle(url, undefined, "testSameOriginImport");
    worker.port.onmessage = function(e) {
      if (e.data.topic == "done") {
        is(e.data.result, "ok", "check relative url in importScripts");
        worker.terminate();
        cbnext();
      }
    }
  },

  testNavigator: function(cbnext) {
    let run = function() {
      let port;
      ononline = function() {
        port.postMessage({topic: "ononline", data: navigator.onLine});
      }
      onoffline = function() {
        port.postMessage({topic: "onoffline", data: navigator.onLine});
      }
      onconnect = function(e) {
        port = e.ports[0];
        port.postMessage({topic: "ready",
                          data: {
                            appName: navigator.appName,
                            appVersion: navigator.appVersion,
                            platform: navigator.platform,
                            userAgent: navigator.userAgent,
                          }
                         });
      }
    }
    let ioService = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService2);
    let oldManage = ioService.manageOfflineStatus;
    let oldOffline = ioService.offline;

    ioService.manageOfflineStatus = false;
    let worker = getFrameWorkerHandle(makeWorkerUrl(run), undefined, "testNavigator");
    let expected_topic = "onoffline";
    let expected_data = false;
    worker.port.onmessage = function(e) {
      is(e.data.topic, "ready");
      for each (let attr in ['appName', 'appVersion', 'platform', 'userAgent']) {
        
        is(typeof e.data.data[attr], "string");
        ok(e.data.data[attr].length > 0);
      }

      worker.port.onmessage = function(e) {
        
        is(e.data.topic, "onoffline");
        is(e.data.data, false);

        
        worker.port.onmessage = function(e) {
          is(e.data.topic, "ononline");
          is(e.data.data, true);
          
          ioService.manageOfflineStatus = oldManage;
          ioService.offline = oldOffline;
          worker.terminate();
          cbnext();
        }
        ioService.offline = false;
      }
      ioService.offline = true;
    }
  },

  testMissingWorker: function(cbnext) {
    
    let url = "https://example.com/browser/toolkit/components/social/test/browser/worker_is_missing.js";
    let worker = getFrameWorkerHandle(url, undefined, "testMissingWorker");
    Services.obs.addObserver(function handleError(subj, topic, data) {
      Services.obs.removeObserver(handleError, "social:frameworker-error");
      is(data, worker._worker.origin, "social:frameworker-error was handled");
      worker.terminate();
      cbnext();
    }, 'social:frameworker-error', false);
    worker.port.onmessage = function(e) {
      ok(false, "social:frameworker-error was handled");
      cbnext();
    }
  },

  testNoConnectWorker: function(cbnext) {
    let worker = getFrameWorkerHandle(makeWorkerUrl(function () {}),
                                      undefined, "testNoConnectWorker");
    Services.obs.addObserver(function handleError(subj, topic, data) {
      Services.obs.removeObserver(handleError, "social:frameworker-error");
      is(data, worker._worker.origin, "social:frameworker-error was handled");
      worker.terminate();
      cbnext();
    }, 'social:frameworker-error', false);
    worker.port.onmessage = function(e) {
      ok(false, "social:frameworker-error was handled");
      cbnext();
    }
  },

  testEmptyWorker: function(cbnext) {
    let worker = getFrameWorkerHandle(makeWorkerUrl(''),
                                      undefined, "testEmptyWorker");
    Services.obs.addObserver(function handleError(subj, topic, data) {
      Services.obs.removeObserver(handleError, "social:frameworker-error");
      is(data, worker._worker.origin, "social:frameworker-error was handled");
      worker.terminate();
      cbnext();
    }, 'social:frameworker-error', false);
    worker.port.onmessage = function(e) {
      ok(false, "social:frameworker-error was handled");
      cbnext();
    }
  },

  testWorkerConnectError: function(cbnext) {
    let run = function () {
      onconnect = function(e) {
        throw new Error("worker failure");
      }
    }
    let worker = getFrameWorkerHandle(makeWorkerUrl(run),
                                      undefined, "testWorkerConnectError");
    Services.obs.addObserver(function handleError(subj, topic, data) {
      Services.obs.removeObserver(handleError, "social:frameworker-error");
      is(data, worker._worker.origin, "social:frameworker-error was handled");
      worker.terminate();
      cbnext();
    }, 'social:frameworker-error', false);
    worker.port.onmessage = function(e) {
      ok(false, "social:frameworker-error was handled");
      cbnext();
    }
  },

  
  
  testCloseFirstSend: function(cbnext) {
    let run = function() {
      let numPings = 0, numCloses = 0;
      onconnect = function(e) {
        let port = e.ports[0];
        port.onmessage = function(e) {
          if (e.data.topic == "ping") {
            numPings += 1;
          } else if (e.data.topic == "social.port-closing") {
            numCloses += 1;
          } else if (e.data.topic == "get-counts") {
            port.postMessage({topic: "result",
                             result: {ping: numPings, close: numCloses}});
          }
        }
      }
    }

    let worker = getFrameWorkerHandle(makeWorkerUrl(run), undefined, "testSendAndClose");
    worker.port.postMessage({topic: "ping"});
    worker.port.close();
    let newPort = getFrameWorkerHandle(makeWorkerUrl(run), undefined, "testSendAndClose").port;
    newPort.onmessage = function(e) {
      if (e.data.topic == "result") {
        is(e.data.result.ping, 1, "the worker got the ping");
        is(e.data.result.close, 1, "the worker got 1 close message");
        worker.terminate();
        cbnext();
      }
    }
    newPort.postMessage({topic: "get-counts"});
  },

  
  
  
  testCloseAfterInit: function(cbnext) {
    let run = function() {
      let numPings = 0, numCloses = 0;
      onconnect = function(e) {
        let port = e.ports[0];
        port.onmessage = function(e) {
          if (e.data.topic == "ping") {
            numPings += 1;
          } else if (e.data.topic == "social.port-closing") {
            numCloses += 1;
          } else if (e.data.topic == "get-counts") {
            port.postMessage({topic: "result",
                             result: {ping: numPings, close: numCloses}});
          } else if (e.data.topic == "get-ready") {
            port.postMessage({topic: "ready"});
          }
        }
      }
    }

    let worker = getFrameWorkerHandle(makeWorkerUrl(run), undefined, "testSendAndClose");
    worker.port.onmessage = function(e) {
      if (e.data.topic == "ready") {
        let newPort = getFrameWorkerHandle(makeWorkerUrl(run), undefined, "testSendAndClose").port;
        newPort.postMessage({topic: "ping"});
        newPort.close();
        worker.port.postMessage({topic: "get-counts"});
      } else if (e.data.topic == "result") {
        is(e.data.result.ping, 1, "the worker got the ping");
        is(e.data.result.close, 1, "the worker got 1 close message");
        worker.terminate();
        cbnext();
      }
    }
    worker.port.postMessage({topic: "get-ready"});
  },
}
