



function makeWorkerUrl(runner) {
  return "data:application/javascript;charset=utf-8," + encodeURI("let run=" + runner.toSource()) + ";run();"
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
    isnot(worker._worker.frame.contentWindow.toString(), "[object ChromeWindow]", "worker window isn't a chrome window");

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
      if (e.data.topic == "hello" && e.data.data.somextrafunction) {
        worker.terminate();
        cbnext();
      }
    }
    worker.port.postMessage({topic: "hello", data: [1,2,3]});
  },

  testArrayUsingBuffer: function(cbnext) {
    let run = function() {
      onconnect = function(e) {
        let port = e.ports[0];
        port.onmessage = function(e) {
          if (e.data.topic == "go") {
            let buffer = new ArrayBuffer(10);
            
            if (new Uint8Array(buffer).length != 10) {
              port.postMessage({topic: "result", reason: "first length was not 10"});
              return;
            }
            let reader = new FileReader();
            reader.onload = function(event) {
              if (new Uint8Array(buffer).length != 10) {
                port.postMessage({topic: "result", reason: "length in onload handler was not 10"});
                return;
              }
              
              port.postMessage({topic: "result", reason: "ok"});
            }
            let blob = new Blob([buffer], {type: "binary"});
            reader.readAsArrayBuffer(blob);
          }
        }
      }
    }
    let worker = getFrameWorkerHandle(makeWorkerUrl(run), undefined, "testArray");
    worker.port.onmessage = function(e) {
      if (e.data.topic == "result") {
        is(e.data.reason, "ok", "check the array worked");
        worker.terminate();
        cbnext();
      }
    }
    worker.port.postMessage({topic: "go"});
  },

  testArrayUsingReader: function(cbnext) {
    let run = function() {
      onconnect = function(e) {
        let port = e.ports[0];
        port.onmessage = function(e) {
          if (e.data.topic == "go") {
            let buffer = new ArrayBuffer(10);
            let reader = new FileReader();
            reader.onload = function(event) {
              try {
                if (new Uint8Array(reader.result).length != 10) {
                  port.postMessage({topic: "result", reason: "length in onload handler was not 10"});
                  return;
                }
                
                port.postMessage({topic: "result", reason: "ok"});
              } catch (ex) {
                port.postMessage({topic: "result", reason: ex.toString()});
              }
            }
            let blob = new Blob([buffer], {type: "binary"});
            reader.readAsArrayBuffer(blob);
          }
        }
      }
    }
    let worker = getFrameWorkerHandle(makeWorkerUrl(run), undefined, "testArray");
    worker.port.onmessage = function(e) {
      if (e.data.topic == "result") {
        is(e.data.reason, "ok", "check the array worked");
        worker.terminate();
        cbnext();
      }
    }
    worker.port.postMessage({topic: "go"});
  },

  testXHR: function(cbnext) {
    
    let url = "https://example.com/browser/toolkit/components/social/test/browser/worker_xhr.js";
    let worker = getFrameWorkerHandle(url, undefined, "testXHR");
    worker.port.onmessage = function(e) {
      if (e.data.topic == "done") {
        is(e.data.result, "ok", "check the xhr test worked");
        worker.terminate();
        cbnext();
      }
    }
  },

  testLocalStorage: function(cbnext) {
    let run = function() {
      onconnect = function(e) {
        let port = e.ports[0];
        try {
          localStorage.setItem("foo", "1");
        } catch(e) {
          port.postMessage({topic: "done", result: "FAILED to set localStorage, " + e.toString() });
          return;
        }

        var ok;
        try {
          ok = localStorage["foo"] == 1;
        } catch (e) {
          port.postMessage({topic: "done", result: "FAILED to read localStorage, " + e.toString() });
          return;
        }
        port.postMessage({topic: "done", result: "ok"});
      }
    }
    let worker = getFrameWorkerHandle(makeWorkerUrl(run), undefined, "testLocalStorage");
    worker.port.onmessage = function(e) {
      if (e.data.topic == "done") {
        is(e.data.result, "ok", "check the localStorage test worked");
        worker.terminate();
        cbnext();
      }
    }
  },

  testBase64: function (cbnext) {
    let run = function() {
      onconnect = function(e) {
        let port = e.ports[0];
        var ok = false;
        try {
          ok = btoa("1234") == "MTIzNA==";
        } catch(e) {
          port.postMessage({topic: "done", result: "FAILED to call btoa, " + e.toString() });
          return;
        }
        if (!ok) {
          port.postMessage({topic: "done", result: "FAILED calling btoa"});
          return;
        }

        try {
          ok = atob("NDMyMQ==") == "4321";
        } catch (e) {
          port.postMessage({topic: "done", result: "FAILED to call atob, " + e.toString() });
          return;
        }
        if (!ok) {
          port.postMessage({topic: "done", result: "FAILED calling atob"});
          return;
        }

        port.postMessage({topic: "done", result: "ok"});
      }
    }
    let worker = getFrameWorkerHandle(makeWorkerUrl(run), undefined, "testBase64");
    worker.port.onmessage = function(e) {
      if (e.data.topic == "done") {
        is(e.data.result, "ok", "check the atob/btoa test worked");
        worker.terminate();
        cbnext();
      }
    }
  },

  testTimeouts: function (cbnext) {
    let run = function() {
      onconnect = function(e) {
        let port = e.ports[0];

        var timeout;
        try {
          timeout = setTimeout(function () {
            port.postMessage({topic: "done", result: "FAILED cancelled timeout was called"});
          }, 100);
        } catch (ex) {
          port.postMessage({topic: "done", result: "FAILED calling setTimeout: " + ex});
          return;
        }

        try {
          clearTimeout(timeout);
        } catch (ex) {
          port.postMessage({topic: "done", result: "FAILED calling clearTimeout: " + ex});
          return;
        }

        var counter = 0;
        try {
          timeout = setInterval(function () {
            if (++counter == 2) {
              clearInterval(timeout);
              setTimeout(function () {
                port.postMessage({topic: "done", result: "ok"});
                return;
              }, 0);
            }
          }, 100);
        } catch (ex) {
          port.postMessage({topic: "done", result: "FAILED calling setInterval: " + ex});
          return;
        }
      }
    }
    let worker = getFrameWorkerHandle(makeWorkerUrl(run), undefined, "testTimeouts");
    worker.port.onmessage = function(e) {
      if (e.data.topic == "done") {
        is(e.data.result, "ok", "check that timeouts worked");
        worker.terminate();
        cbnext();
      }
    }
  },

  testWebSocket: function (cbnext) {
    let run = function() {
      onconnect = function(e) {
        let port = e.ports[0];

        try {
          var exampleSocket = new WebSocket("ws://www.example.com/socketserver");
        } catch (e) {
          port.postMessage({topic: "done", result: "FAILED calling WebSocket constructor: " + e});
          return;
        }

        port.postMessage({topic: "done", result: "ok"});
      }
    }
    let worker = getFrameWorkerHandle(makeWorkerUrl(run), undefined, "testWebSocket");
    worker.port.onmessage = function(e) {
      if (e.data.topic == "done") {
        is(e.data.result, "ok", "check that websockets worked");
        worker.terminate();
        cbnext();
      }
    }
  },

  testSameOriginImport: function(cbnext) {
    let run = function() {
      onconnect = function(e) {
        let port = e.ports[0];
        port.onmessage = function(e) {
          if (e.data.topic == "ping") {
            try {
              importScripts("http://foo.bar/error");
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
  }
}
