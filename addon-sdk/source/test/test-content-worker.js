



"use stirct";

const { Cc, Ci } = require("chrome");
const { setTimeout } = require("sdk/timers");
const { LoaderWithHookedConsole } = require("sdk/test/loader");
const { Worker } = require("sdk/content/worker");

const DEFAULT_CONTENT_URL = "data:text/html;charset=utf-8,foo";

function makeWindow(contentURL) {
  let content =
    "<?xml version=\"1.0\"?>" +
    "<window " +
    "xmlns=\"http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul\">" +
    "<script>var documentValue=true;</script>" +
    "</window>";
  var url = "data:application/vnd.mozilla.xul+xml;charset=utf-8," +
            encodeURIComponent(content);
  var features = ["chrome", "width=10", "height=10"];

  return Cc["@mozilla.org/embedcomp/window-watcher;1"].
         getService(Ci.nsIWindowWatcher).
         openWindow(null, url, null, features.join(","), null);
}


function listenOnce(node, eventName, callback) {
  node.addEventListener(eventName, function onevent(event) {
    node.removeEventListener(eventName, onevent, true);
    callback(node);
  }, true);
}


function loadAndWait(browser, url, callback) {
  listenOnce(browser, "load", callback);
  
  
  setTimeout(function () {
    browser.loadURI(url);
  }, 0);
}







function WorkerTest(url, callback) {
  return function testFunction(assert, done) {
    let chromeWindow = makeWindow();
    chromeWindow.addEventListener("load", function onload() {
      chromeWindow.removeEventListener("load", onload, true);
      let browser = chromeWindow.document.createElement("browser");
      browser.setAttribute("type", "content");
      chromeWindow.document.documentElement.appendChild(browser);
      
      listenOnce(browser, "load", function onAboutBlankLoad() {
        
        loadAndWait(browser, url, function onDocumentLoaded() {
          callback(assert, browser, function onTestDone() {
            chromeWindow.close();
            done();
          });
        });
      });
    }, true);
  };
}

exports["test:sample"] = WorkerTest(
  DEFAULT_CONTENT_URL,
  function(assert, browser, done) {

    assert.notEqual(browser.contentWindow.location.href, "about:blank",
                        "window is now on the right document");

    let window = browser.contentWindow
    let worker =  Worker({
      window: window,
      contentScript: "new " + function WorkerScope() {
        
        let myLocation = window.location.toString();
        self.on("message", function(data) {
          if (data == "hi!")
            self.postMessage("bye!");
        });
      },
      contentScriptWhen: "ready",
      onMessage: function(msg) {
        assert.equal("bye!", msg);
        assert.equal(worker.url, window.location.href,
                         "worker.url still works");
        done();
      }
    });

    assert.equal(worker.url, window.location.href,
                     "worker.url works");
    worker.postMessage("hi!");
  }
);

exports["test:emit"] = WorkerTest(
  DEFAULT_CONTENT_URL,
  function(assert, browser, done) {

    let worker =  Worker({
        window: browser.contentWindow,
        contentScript: "new " + function WorkerScope() {
          
          self.port.on("addon-to-content", function (data) {
            self.port.emit("content-to-addon", data);
          });

          
          
          
          if (typeof once != "undefined")
            self.postMessage("`once` is in globals");
          if (typeof emit != "undefined")
            self.postMessage("`emit` is in globals");

        },
        onMessage: function(msg) {
          assert.fail("Got an unexpected message : "+msg);
        }
      });

    
    worker.port.on("content-to-addon", function (data) {
      assert.equal(data, "event data");
      done();
    });
    worker.port.emit("addon-to-content", "event data");
  }
);

exports["test:emit hack message"] = WorkerTest(
  DEFAULT_CONTENT_URL,
  function(assert, browser, done) {
    let worker =  Worker({
        window: browser.contentWindow,
        contentScript: "new " + function WorkerScope() {
          
          self.port.on("message", function (data) {
            self.port.emit("message", data);
          });
          
          self.on("message", function (data) {
            self.postMessage("message", data);
          });
        },
        onError: function(e) {
          assert.fail("Got exception: "+e);
        }
      });

    worker.port.on("message", function (data) {
      assert.equal(data, "event data");
      done();
    });
    worker.on("message", function (data) {
      assert.fail("Got an unexpected message : "+msg);
    });
    worker.port.emit("message", "event data");
  }
);

exports["test:n-arguments emit"] = WorkerTest(
  DEFAULT_CONTENT_URL,
  function(assert, browser, done) {
    let repeat = 0;
    let worker =  Worker({
        window: browser.contentWindow,
        contentScript: "new " + function WorkerScope() {
          
          self.port.on("addon-to-content", function (a1, a2, a3) {
            self.port.emit("content-to-addon", a1, a2, a3);
          });
        }
      });

    
    worker.port.on("content-to-addon", function (arg1, arg2, arg3) {
      if (!repeat++) {
        this.emit("addon-to-content", "first argument", "second", "third");
      } else {
        assert.equal(arg1, "first argument");
        assert.equal(arg2, "second");
        assert.equal(arg3, "third");
        done();
      }
    });
    worker.port.emit("addon-to-content", "first argument", "second", "third");
  }
);

exports["test:post-json-values-only"] = WorkerTest(
  DEFAULT_CONTENT_URL,
  function(assert, browser, done) {

    let worker =  Worker({
        window: browser.contentWindow,
        contentScript: "new " + function WorkerScope() {
          self.on("message", function (message) {
            self.postMessage([ message.fun === undefined,
                               typeof message.w,
                               message.w && "port" in message.w,
                               message.w.url,
                               Array.isArray(message.array),
                               JSON.stringify(message.array)]);
          });
        }
      });

    
    let array = [1, 2, 3];
    worker.on("message", function (message) {
      assert.ok(message[0], "function becomes undefined");
      assert.equal(message[1], "object", "object stays object");
      assert.ok(message[2], "object's attributes are enumerable");
      assert.equal(message[3], DEFAULT_CONTENT_URL,
                       "jsonable attributes are accessible");
      
      assert.ok(message[4], "Array keeps being an array");
      assert.equal(message[5], JSON.stringify(array),
                       "Array is correctly serialized");
      done();
    });
    worker.postMessage({ fun: function () {}, w: worker, array: array });
  }
);

exports["test:emit-json-values-only"] = WorkerTest(
  DEFAULT_CONTENT_URL,
  function(assert, browser, done) {

    let worker =  Worker({
        window: browser.contentWindow,
        contentScript: "new " + function WorkerScope() {
          
          self.port.on("addon-to-content", function (fun, w, obj, array) {
            self.port.emit("content-to-addon", [
                            fun === null,
                            typeof w,
                            "port" in w,
                            w.url,
                            "fun" in obj,
                            Object.keys(obj.dom).length,
                            Array.isArray(array),
                            JSON.stringify(array)
                          ]);
          });
        }
      });

    
    let array = [1, 2, 3];
    worker.port.on("content-to-addon", function (result) {
      assert.ok(result[0], "functions become null");
      assert.equal(result[1], "object", "objects stay objects");
      assert.ok(result[2], "object's attributes are enumerable");
      assert.equal(result[3], DEFAULT_CONTENT_URL,
                       "json attribute is accessible");
      assert.ok(!result[4], "function as object attribute is removed");
      assert.equal(result[5], 0, "DOM nodes are converted into empty object");
      
      assert.ok(result[6], "Array keeps being an array");
      assert.equal(result[7], JSON.stringify(array),
                       "Array is correctly serialized");
      done();
    });

    let obj = {
      fun: function () {},
      dom: browser.contentWindow.document.createElement("div")
    };
    worker.port.emit("addon-to-content", function () {}, worker, obj, array);
  }
);

exports["test:content is wrapped"] = WorkerTest(
  "data:text/html;charset=utf-8,<script>var documentValue=true;</script>",
  function(assert, browser, done) {

    let worker =  Worker({
      window: browser.contentWindow,
      contentScript: "new " + function WorkerScope() {
        self.postMessage(!window.documentValue);
      },
      contentScriptWhen: "ready",
      onMessage: function(msg) {
        assert.ok(msg,
          "content script has a wrapped access to content document");
        done();
      }
    });
  }
);

exports["test:chrome is unwrapped"] = function(assert, done) {
  let window = makeWindow();

  listenOnce(window, "load", function onload() {

    let worker =  Worker({
      window: window,
      contentScript: "new " + function WorkerScope() {
        self.postMessage(window.documentValue);
      },
      contentScriptWhen: "ready",
      onMessage: function(msg) {
        assert.ok(msg,
          "content script has an unwrapped access to chrome document");
        window.close();
        done();
      }
    });

  });
}

exports["test:nothing is leaked to content script"] = WorkerTest(
  DEFAULT_CONTENT_URL,
  function(assert, browser, done) {

    let worker =  Worker({
      window: browser.contentWindow,
      contentScript: "new " + function WorkerScope() {
        self.postMessage([
          "ContentWorker" in window,
          "UNWRAP_ACCESS_KEY" in window,
          "getProxyForObject" in window
        ]);
      },
      contentScriptWhen: "ready",
      onMessage: function(list) {
        assert.ok(!list[0], "worker API contrustor isn't leaked");
        assert.ok(!list[1], "Proxy API stuff isn't leaked 1/2");
        assert.ok(!list[2], "Proxy API stuff isn't leaked 2/2");
        done();
      }
    });
  }
);

exports["test:ensure console.xxx works in cs"] = WorkerTest(
  DEFAULT_CONTENT_URL,
  function(assert, browser, done) {
    let { loader } = LoaderWithHookedConsole(module, onMessage);

    
    let calls = [];
    function onMessage(type, msg) {
      assert.equal(type, msg,
                       "console.xxx(\"xxx\"), i.e. message is equal to the " +
                       "console method name we are calling");
      calls.push(msg);
    }

    
    let worker =  loader.require("sdk/content/worker").Worker({
      window: browser.contentWindow,
      contentScript: "new " + function WorkerScope() {
        console.log("log");
        console.info("info");
        console.warn("warn");
        console.error("error");
        console.debug("debug");
        console.exception("exception");
        self.postMessage();
      },
      onMessage: function() {
        
        assert.equal(JSON.stringify(calls),
                         JSON.stringify(["log", "info", "warn", "error", "debug", "exception"]),
                         "console has been called successfully, in the expected order");
        done();
      }
    });
  }
);


exports["test:setTimeout can\"t be cancelled by content"] = WorkerTest(
  "data:text/html;charset=utf-8,<script>var documentValue=true;</script>",
  function(assert, browser, done) {

    let worker =  Worker({
      window: browser.contentWindow,
      contentScript: "new " + function WorkerScope() {
        let id = setTimeout(function () {
          self.postMessage("timeout");
        }, 100);
        unsafeWindow.eval("clearTimeout("+id+");");
      },
      contentScriptWhen: "ready",
      onMessage: function(msg) {
        assert.ok(msg,
          "content didn't managed to cancel our setTimeout");
        done();
      }
    });
  }
);

exports["test:clearTimeout"] = WorkerTest(
  "data:text/html;charset=utf-8,clear timeout",
  function(assert, browser, done) {
    let worker = Worker({
      window: browser.contentWindow,
      contentScript: "new " + function WorkerScope() {
        let id1 = setTimeout(function() {
          self.postMessage("failed");
        }, 10);
        let id2 = setTimeout(function() {
          self.postMessage("done");
        }, 100);
        clearTimeout(id1);
      },
      contentScriptWhen: "ready",
      onMessage: function(msg) {
        if (msg === "failed") {
          assert.fail("failed to cancel timer");
        } else {
          assert.pass("timer cancelled");
          done();
        }
      }
    });
  }
);

exports["test:clearInterval"] = WorkerTest(
  "data:text/html;charset=utf-8,clear timeout",
  function(assert, browser, done) {
    let called = 0;
    let worker = Worker({
      window: browser.contentWindow,
      contentScript: "new " + function WorkerScope() {
        let id = setInterval(function() {
          self.postMessage("intreval")
          clearInterval(id)
          setTimeout(function() {
            self.postMessage("done")
          }, 100)
        }, 10);
      },
      contentScriptWhen: "ready",
      onMessage: function(msg) {
        if (msg === "intreval") {
          called = called + 1;
          if (called > 1) assert.fail("failed to cancel timer");
        } else {
          assert.pass("interval cancelled");
          done();
        }
      }
    });
  }
)

exports["test:setTimeout are unregistered on content unload"] = WorkerTest(
  DEFAULT_CONTENT_URL,
  function(assert, browser, done) {

    let originalWindow = browser.contentWindow;
    let worker =  Worker({
      window: browser.contentWindow,
      contentScript: "new " + function WorkerScope() {
        document.title = "ok";
        let i = 0;
        setInterval(function () {
          document.title = i++;
        }, 10);
      },
      contentScriptWhen: "ready"
    });

    
    
    
    setTimeout(function () {
      
      
      let url2 = "data:text/html;charset=utf-8,<title>final</title>";
      loadAndWait(browser, url2, function onload() {
        let titleAfterLoad = originalWindow.document.title;
        
        setTimeout(function () {
          assert.equal(browser.contentDocument.title, "final",
                           "New document has not been modified");
          assert.equal(originalWindow.document.title, titleAfterLoad,
                           "Nor previous one");

          done();
        }, 100);
      });
    }, 100);
  }
);

exports['test:check window attribute in iframes'] = WorkerTest(
  DEFAULT_CONTENT_URL,
  function(assert, browser, done) {

    
    let contentWin = browser.contentWindow;
    let contentDoc = contentWin.document;
    let iframe = contentDoc.createElement("iframe");
    contentDoc.body.appendChild(iframe);

    listenOnce(iframe, "load", function onload() {

      
      let iframeDoc = iframe.contentWindow.document;
      let subIframe = iframeDoc.createElement("iframe");
      iframeDoc.body.appendChild(subIframe);

      listenOnce(subIframe, "load", function onload() {
        subIframe.removeEventListener("load", onload, true);

        
        let worker =  Worker({
          window: subIframe.contentWindow,
          contentScript: 'new ' + function WorkerScope() {
            self.postMessage([
              window.top !== window,
              frameElement,
              window.parent !== window,
              top.location.href,
              parent.location.href,
            ]);
          },
          onMessage: function(msg) {
            assert.ok(msg[0], "window.top != window");
            assert.ok(msg[1], "window.frameElement is defined");
            assert.ok(msg[2], "window.parent != window");
            assert.equal(msg[3], contentWin.location.href,
                             "top.location refers to the toplevel content doc");
            assert.equal(msg[4], iframe.contentWindow.location.href,
                             "parent.location refers to the first iframe doc");
            done();
          }
        });

      });
      subIframe.setAttribute("src", "data:text/html;charset=utf-8,bar");

    });
    iframe.setAttribute("src", "data:text/html;charset=utf-8,foo");
  }
);

exports['test:check window attribute in toplevel documents'] = WorkerTest(
  DEFAULT_CONTENT_URL,
  function(assert, browser, done) {

    let worker =  Worker({
      window: browser.contentWindow,
      contentScript: 'new ' + function WorkerScope() {
        self.postMessage([
          window.top === window,
          frameElement,
          window.parent === window
        ]);
      },
      onMessage: function(msg) {
        assert.ok(msg[0], "window.top == window");
        assert.ok(!msg[1], "window.frameElement is null");
        assert.ok(msg[2], "window.parent == window");
        done();
      }
    });
  }
);

exports["test:check worker API with page history"] = WorkerTest(
  DEFAULT_CONTENT_URL,
  function(assert, browser, done) {
    let url2 = "data:text/html;charset=utf-8,bar";

    loadAndWait(browser, url2, function () {
      let worker =  Worker({
        window: browser.contentWindow,
        contentScript: "new " + function WorkerScope() {
          
          
          self.on("pagehide", function () {
            setTimeout(function () {
              self.postMessage("timeout restored");
            }, 0);
          });
        },
        contentScriptWhen: "start"
      });

      
      worker.postMessage("ok");

      
      
      setTimeout(function () {
        browser.goBack();
      }, 0);

      
      browser.addEventListener("pagehide", function onpagehide() {
        browser.removeEventListener("pagehide", onpagehide, false);
        

        assert.throws(
            function () { worker.postMessage("data"); },
            /The page is currently hidden and can no longer be used/,
            "postMessage should throw when the page is hidden in history"
            );

        assert.throws(
            function () { worker.port.emit("event"); },
            /The page is currently hidden and can no longer be used/,
            "port.emit should throw when the page is hidden in history"
            );

        
        
        
        
        
        
        setTimeout(function () {
          worker.on("message", function (data) {
            assert.ok(data, "timeout restored");
            done();
          });
          browser.goForward();
        }, 500);

      }, false);
    });

  }
);

exports["test:global postMessage"] = WorkerTest(
  DEFAULT_CONTENT_URL,
  function(assert, browser, done) {
    let { loader } = LoaderWithHookedConsole(module, onMessage);

    
    let seenMessages = 0;
    function onMessage(type, message) {
      seenMessages++;
      assert.equal(type, "error", "Should be an error");
      assert.equal(message, "DEPRECATED: The global `postMessage()` function in " +
                            "content scripts is deprecated in favor of the " +
                            "`self.postMessage()` function, which works the same. " +
                            "Replace calls to `postMessage()` with calls to " +
                            "`self.postMessage()`." +
                            "For more info on `self.on`, see " +
                            "<https://addons.mozilla.org/en-US/developers/docs/sdk/latest/dev-guide/addon-development/web-content.html>.",
                            "Should have seen the deprecation message")
    }

    assert.notEqual(browser.contentWindow.location.href, "about:blank",
                        "window is now on the right document");

    let window = browser.contentWindow
    let worker = loader.require("sdk/content/worker").Worker({
      window: window,
      contentScript: "new " + function WorkerScope() {
        postMessage("success");
      },
      contentScriptWhen: "ready",
      onMessage: function(msg) {
        assert.equal("success", msg, "Should have seen the right postMessage call");
        assert.equal(1, seenMessages, "Should have seen the deprecation message");
        done();
      }
    });

    assert.equal(worker.url, window.location.href,
                     "worker.url works");
    worker.postMessage("hi!");
  }
);

if (require("sdk/system/xul-app").is("Fennec")) {
  module.exports = {
    "test Unsupported Test": function UnsupportedTest (assert) {
        assert.pass(
          "Skipping this test until Fennec support is implemented." +
          "See bug 806817");
    }
  }
}

require("test").run(exports);
