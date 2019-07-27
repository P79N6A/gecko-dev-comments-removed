



const hiddenFrames = require("sdk/frame/hidden-frame");
const { create: makeFrame } = require("sdk/frame/utils");
const { window } = require("sdk/addon/window");
const { Loader } = require('sdk/test/loader');
const { URL } = require("sdk/url");
const testURI = require("./fixtures").url("test.html");
const testHost = URL(testURI).scheme + '://' + URL(testURI).host;





function createProxyTest(html, callback) {
  return function (assert, done) {
    let url = 'data:text/html;charset=utf-8,' + encodeURIComponent(html);
    let principalLoaded = false;

    let element = makeFrame(window.document, {
      nodeName: "iframe",
      type: "content",
      allowJavascript: true,
      allowPlugins: true,
      allowAuth: true,
      uri: testURI
    });

    element.addEventListener("DOMContentLoaded", onDOMReady, false);

    function onDOMReady() {
      
      if (!principalLoaded) {
        element.setAttribute("src", url);
        principalLoaded = true;
        return;
      }

      assert.equal(element.getAttribute("src"), url, "correct URL loaded");
      element.removeEventListener("DOMContentLoaded", onDOMReady,
                                                  false);
      let xrayWindow = element.contentWindow;
      let rawWindow = xrayWindow.wrappedJSObject;

      let isDone = false;
      let helper = {
        xrayWindow: xrayWindow,
        rawWindow: rawWindow,
        createWorker: function (contentScript) {
          return createWorker(assert, xrayWindow, contentScript, helper.done);
        },
        done: function () {
          if (isDone)
            return;
          isDone = true;
          element.parentNode.removeChild(element);
          done();
        }
      };
      callback(helper, assert);
    }
  };
}

function createWorker(assert, xrayWindow, contentScript, done) {
  let loader = Loader(module);
  let Worker = loader.require("sdk/content/worker").Worker;
  let worker = Worker({
    window: xrayWindow,
    contentScript: [
      'new ' + function () {
        assert = function assert(v, msg) {
          self.port.emit("assert", {assertion:v, msg:msg});
        }
        done = function done() {
          self.port.emit("done");
        }
      },
      contentScript
    ]
  });

  worker.port.on("done", done);
  worker.port.on("assert", function (data) {
    assert.ok(data.assertion, data.msg);
  });

  return worker;
}



let html = "<script>var documentGlobal = true</script>";

exports["test Create Proxy Test"] = createProxyTest(html, function (helper, assert) {
  
  
  assert.ok(helper.rawWindow.documentGlobal,
              "You have access to a raw window reference via `helper.rawWindow`");
  assert.ok(!("documentGlobal" in helper.xrayWindow),
              "You have access to an XrayWrapper reference via `helper.xrayWindow`");

  
  
  helper.done();
});

exports["test Create Proxy Test With Worker"] = createProxyTest("", function (helper) {

  helper.createWorker(
    "new " + function WorkerScope() {
      assert(true, "You can do assertions in your content script");
      
      
      done();
    }
  );

});

exports["test Create Proxy Test With Events"] = createProxyTest("", function (helper, assert) {

  let worker = helper.createWorker(
    "new " + function WorkerScope() {
      self.port.emit("foo");
    }
  );

  worker.port.on("foo", function () {
    assert.pass("You can use events");
    
    helper.done();
  });

});


































let html = '<iframe id="iframe" name="test" src="data:text/html;charset=utf-8," />';
exports["test postMessage"] = createProxyTest(html, function (helper, assert) {
  let ifWindow = helper.xrayWindow.document.getElementById("iframe").contentWindow;
  
  
  ifWindow.addEventListener("message", function listener(event) {
    ifWindow.removeEventListener("message", listener, false);
    
    
    
    
    assert.strictEqual(event.source, helper.xrayWindow,
                      "event.source is the top window");
    assert.equal(event.origin, testHost, "origin matches testHost");

    assert.equal(event.data, "{\"foo\":\"bar\\n \\\"escaped\\\".\"}",
                     "message data is correct");

    helper.done();
  }, false);

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      var json = JSON.stringify({foo : "bar\n \"escaped\"."});

      document.getElementById("iframe").contentWindow.postMessage(json, "*");
    }
  );
});

let html = '<input id="input2" type="checkbox" />';
exports["test Object Listener"] = createProxyTest(html, function (helper) {

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      
      let input = document.getElementById("input2");
      let myClickListener = {
        called: false,
        handleEvent: function(event) {
          assert(this === myClickListener, "`this` is the original object");
          assert(!this.called, "called only once");
          this.called = true;
          assert(event.target, input, "event.target is the wrapped window");
          done();
        }
      };

      window.addEventListener("click", myClickListener, true);
      input.click();
      window.removeEventListener("click", myClickListener, true);
    }
  );

});

exports["test Object Listener 2"] = createProxyTest("", function (helper) {

  helper.createWorker(
    ('new ' + function ContentScriptScope() {
      
      let testHost = "TOKEN";
      
      let myMessageListener = {
        called: false,
        handleEvent: function(event) {
          window.removeEventListener("message", myMessageListener, true);

          assert(this == myMessageListener, "`this` is the original object");
          assert(!this.called, "called only once");
          this.called = true;
          assert(event.target == document.defaultView, "event.target is the wrapped window");
          assert(event.source == document.defaultView, "event.source is the wrapped window");
          assert(event.origin == testHost, "origin matches testHost");
          assert(event.data == "ok", "message data is correct");
          done();
        }
      };

      window.addEventListener("message", myMessageListener, true);
      document.defaultView.postMessage("ok", '*');
    }
  ).replace("TOKEN", testHost));

});

let html = '<input id="input" type="text" /><input id="input3" type="checkbox" />' +
             '<input id="input2" type="checkbox" />';

exports.testStringOverload = createProxyTest(html, function (helper, assert) {
  
  let originalString = "string";
  let p = Proxy.create({
    get: function(receiver, name) {
      if (name == "binded")
        return originalString.toString.bind(originalString);
      return originalString[name];
    }
  });
  assert.throws(function () {
    p.toString();
  },
  /toString method called on incompatible Proxy/,
  "toString can't be called with this being the proxy");
  assert.equal(p.binded(), "string", "but it works if we bind this to the original string");

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      
      
      
      
      
      
      String.prototype.update = function () {
        assert(typeof this == "object", "in update, `this` is an object");
        assert(this.toString() == "input", "in update, `this.toString works");
        return document.querySelectorAll(this);
      };
      assert("input".update().length == 3, "String.prototype overload works");
      done();
    }
  );
});

exports["test MozMatchedSelector"] = createProxyTest("", function (helper) {
  helper.createWorker(
    'new ' + function ContentScriptScope() {
      
      
      
      assert(document.createElement( "div" ).mozMatchesSelector("div"),
             "mozMatchesSelector works while being called from the node");
      assert(document.documentElement.mozMatchesSelector.call(
               document.createElement( "div" ),
               "div"
             ),
             "mozMatchesSelector works while being called from a " +
             "function reference to " +
             "document.documentElement.mozMatchesSelector.call");
      done();
    }
  );
});

exports["test Events Overload"] = createProxyTest("", function (helper) {

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      let proto = window.document.createEvent('HTMLEvents').__proto__;
      window.Event.prototype = proto;
      let event = document.createEvent('HTMLEvents');
      assert(event !== proto, "Event should not be equal to its prototype");
      event.initEvent('dataavailable', true, true);
      assert(event.type === 'dataavailable', "Events are working fine");
      done();
    }
  );

});

exports["test Nested Attributes"] = createProxyTest("", function (helper) {

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      
      
      
      
      
      let o = {sandboxObject:true};
      window.nested = o;
      o.foo = true;
      assert(o === window.nested, "Nested attribute to sandbox object should not be proxified");
      window.nested = document;
      assert(window.nested === document, "Nested attribute to proxy should not be double proxified");
      done();
    }
  );

});

exports["test Form nodeName"] = createProxyTest("", function (helper) {

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      let body = document.body;
      
      let form = document.createElement("form");
      let input = document.createElement("input");
      input.setAttribute("name", "test");
      form.appendChild(input);
      body.appendChild(form);
      assert(form.test == input, "form[nodeName] is valid");
      body.removeChild(form);
      done();
    }
  );

});

exports["test localStorage"] = createProxyTest("", function (helper, assert) {

  let worker = helper.createWorker(
    'new ' + function ContentScriptScope() {
      
      assert(window.localStorage, "has access to localStorage");
      window.localStorage.name = 1;
      assert(window.localStorage.name == 1, "localStorage appears to work");

      self.port.on("step2", function () {
        window.localStorage.clear();
        assert(window.localStorage.name == undefined, "localStorage really, really works");
        done();
      });
      self.port.emit("step1");
    }
  );

  worker.port.on("step1", function () {
    assert.equal(helper.rawWindow.localStorage.name, 1, "localStorage really works");
    worker.port.emit("step2");
  });

});

exports["test Auto Unwrap Custom Attributes"] = createProxyTest("", function (helper) {

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      let body = document.body;
      
      let object = {custom: true, enumerable: false};
      body.customAttribute = object;
      assert(object === body.customAttribute, "custom JS attributes are not wrapped");
      done();
    }
  );

});

exports["test Object Tag"] = createProxyTest("", function (helper) {

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      
      let flash = document.createElement("object");
      assert(typeof flash == "function", "<object> is typeof 'function'");
      assert(flash.toString().match(/\[object HTMLObjectElement.*\]/), "<object> is HTMLObjectElement");
      assert("setAttribute" in flash, "<object> has a setAttribute method");
      done();
    }
  );

});

exports["test Highlight toString Behavior"] = createProxyTest("", function (helper, assert) {
  
  
  
  function f() {};
  let funToString = Object.prototype.toString.call(f);
  assert.ok(/\[object Function.*\]/.test(funToString), "functions are functions 1");

  
  let strToString = helper.rawWindow.Object.prototype.toString.call("");
  assert.ok(/\[object String.*\]/.test(strToString), "strings are strings");

  let o = {__exposedProps__:{}};
  let objToString = helper.rawWindow.Object.prototype.toString.call(o);
  assert.ok(/\[object Object.*\]/.test(objToString), "objects are objects");

  
  
  let f2 = helper.rawWindow.eval("(function () {})");
  let funToString2 = helper.rawWindow.Object.prototype.toString.call(f2);
  assert.ok(/\[object Function.*\]/.test(funToString2), "functions are functions 2");

  helper.done();
});

exports["test Document TagName"] = createProxyTest("", function (helper) {

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      let body = document.body;
      
      let div = document.createElement("div");
      div.setAttribute("name", "test");
      body.appendChild(div);
      assert(!document.test, "document[divName] is undefined");
      body.removeChild(div);

      let form = document.createElement("form");
      form.setAttribute("name", "test");
      body.appendChild(form);
      assert(document.test == form, "document[formName] is valid");
      body.removeChild(form);

      let img = document.createElement("img");
      img.setAttribute("name", "test");
      body.appendChild(img);
      assert(document.test == img, "document[imgName] is valid");
      body.removeChild(img);
      done();
    }
  );

});

let html = '<iframe id="iframe" name="test" src="data:text/html;charset=utf-8," />';
exports["test Window Frames"] = createProxyTest(html, function (helper) {

  helper.createWorker(
    'let glob = this; new ' + function ContentScriptScope() {
      
      let iframe = document.getElementById("iframe");
      
      
      assert(window.test == iframe.contentWindow, "window[frameName] is valid");
      done();
    }
  );

});

exports["test Collections"] = createProxyTest("", function (helper) {

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      
      
      let body = document.body;
      let div = document.createElement("div");
      body.appendChild(div);
      div.innerHTML = "<table><tr><td style='padding:0;border:0;display:none'></td><td>t</td></tr></table>";
      let tds = div.getElementsByTagName("td");
      assert(tds[0] == tds[0], "We can get array element multiple times");
      body.removeChild(div);
      done();
    }
  );

});

let html = '<input id="input" type="text" /><input id="input3" type="checkbox" />' +
             '<input id="input2" type="checkbox" />';
exports["test Collections 2"] = createProxyTest(html, function (helper) {

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      
      let body = document.body;
      let inputs = body.getElementsByTagName("input");
      assert(body.childNodes.length == 3, "body.childNodes length is correct");
      assert(inputs.length == 3, "inputs.length is correct");
      assert(body.childNodes[0] == inputs[0], "body.childNodes[0] is correct");
      assert(body.childNodes[1] == inputs[1], "body.childNodes[1] is correct");
      assert(body.childNodes[2] == inputs[2], "body.childNodes[2] is correct");
      let count = 0;
      for(let i in body.childNodes) {
        count++;
      }

      assert(count >= 3, "body.childNodes is iterable");
      done();
    }
  );

});

exports["test XMLHttpRequest"] = createProxyTest("", function (helper) {

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      
      
      assert(new window.XMLHttpRequest(), "we are able to instantiate XMLHttpRequest object");
      done();
    }
  );

});

exports["test XPathResult"] = createProxyTest("", function (helper, assert) {
  const XPathResultTypes = ["ANY_TYPE",
                            "NUMBER_TYPE", "STRING_TYPE", "BOOLEAN_TYPE",
                            "UNORDERED_NODE_ITERATOR_TYPE",
                            "ORDERED_NODE_ITERATOR_TYPE",
                            "UNORDERED_NODE_SNAPSHOT_TYPE",
                            "ORDERED_NODE_SNAPSHOT_TYPE",
                            "ANY_UNORDERED_NODE_TYPE",
                            "FIRST_ORDERED_NODE_TYPE"];

  
  let xpcXPathResult = helper.xrayWindow.XPathResult;

  XPathResultTypes.forEach(function(type, i) {
    assert.equal(xpcXPathResult.wrappedJSObject[type],
                     helper.rawWindow.XPathResult[type],
                     "XPathResult's constants are valid on unwrapped node");

    assert.equal(xpcXPathResult[type], i,
                     "XPathResult's constants are defined on " +
                     "XPCNativeWrapper (platform bug #)");
  });

  let value = helper.rawWindow.XPathResult.UNORDERED_NODE_SNAPSHOT_TYPE;
  let worker = helper.createWorker(
    'new ' + function ContentScriptScope() {
      self.port.on("value", function (value) {
        
        assert(window.XPathResult.UNORDERED_NODE_SNAPSHOT_TYPE === value,
               "XPathResult works correctly on Proxies");
        done();
      });
    }
  );
  worker.port.emit("value", value);
});

exports["test Prototype Inheritance"] = createProxyTest("", function (helper) {

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      
      
      let event1 = document.createEvent( 'MouseEvents' );
      event1.initEvent( "click", true, true );
      let event2 = document.createEvent( 'MouseEvents' );
      event2.initEvent( "click", true, true );
      assert(event2.type == "click", "We are able to create an event");
      done();
    }
  );

});

exports["test Functions"] = createProxyTest("", function (helper) {

  helper.rawWindow.callFunction = function callFunction(f) f();
  helper.rawWindow.isEqual = function isEqual(a, b) a == b;
  
  
  helper.rawWindow.callFunction.__exposedProps__ = {__proxy: 'rw'};
  helper.rawWindow.isEqual.__exposedProps__ = {__proxy: 'rw'};

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      
      let closure2 = function () {return "ok";};
      assert(window.wrappedJSObject.callFunction(closure2) == "ok", "Function references work");

      
      let closure = function () {};
      assert(window.wrappedJSObject.isEqual(closure, closure), "Function references are cached before being wrapped to native");
      done();
    }
  );

});

let html = '<input id="input2" type="checkbox" />';
exports["test Listeners"] = createProxyTest(html, function (helper) {

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      
      let input = document.getElementById("input2");
      assert(input, "proxy.getElementById works");

      function onclick() {};
      input.onclick = onclick;
      assert(input.onclick === onclick, "on* attributes are equal to original function set");

      let addEventListenerCalled = false;
      let expandoCalled = false;
      input.addEventListener("click", function onclick(event) {
        input.removeEventListener("click", onclick, true);

        assert(!addEventListenerCalled, "closure given to addEventListener is called once");
        if (addEventListenerCalled)
          return;
        addEventListenerCalled = true;

        assert(!event.target.ownerDocument.defaultView.documentGlobal, "event object is still wrapped and doesn't expose document globals");

        let input2 = document.getElementById("input2");

        input.onclick = function (event) {
          input.onclick = null;
          assert(!expandoCalled, "closure set to expando is called once");
          if (expandoCalled) return;
          expandoCalled = true;

          assert(!event.target.ownerDocument.defaultView.documentGlobal, "event object is still wrapped and doesn't expose document globals");

          setTimeout(function () {
            input.click();
            done();
          }, 0);

        }

        setTimeout(function () {
          input.click();
        }, 0);

      }, true);

      input.click();
    }
  );

});

exports["test MozRequestAnimationFrame"] = createProxyTest("", function (helper) {

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      window.mozRequestAnimationFrame(function callback() {
        assert(callback == this, "callback is equal to `this`");
        done();
      });
    }
  );

});

exports["testGlobalScope"] = createProxyTest("", function (helper) {

  helper.createWorker(
    'let toplevelScope = true;' +
    'assert(window.toplevelScope, "variables in toplevel scope are set to `window` object");' +
    'assert(this.toplevelScope, "variables in toplevel scope are set to `this` object");' +
    'done();'
  );

});



exports["test Cross Domain Iframe"] = createProxyTest("", function (helper) {
  let serverPort = 8099;
  let server = require("./lib/httpd").startServerAsync(serverPort);
  server.registerPathHandler("/", function handle(request, response) {
    
    
    let content = "<html><head><meta charset='utf-8'></head>\n";
    content += "<script>\n";
    content += "  window.addEventListener('message', function (event) {\n";
    content += "    parent.postMessage(event.data + ' world', '*');\n";
    content += "  }, true);\n";
    content += "</script>\n";
    content += "<body></body>\n";
    content += "</html>\n";
    response.write(content);
  });

  let worker = helper.createWorker(
    'new ' + function ContentScriptScope() {
      
      self.on("message", function (url) {
        
        let iframe = document.createElement("iframe");
        iframe.addEventListener("load", function onload() {
          iframe.removeEventListener("load", onload, true);
          try {
            
            window.addEventListener("message", function onmessage(event) {
              window.removeEventListener("message", onmessage, true);

              assert(event.data == "hello world", "COW works properly");
              self.port.emit("end");
            }, true);
            iframe.contentWindow.postMessage("hello", "*");
          } catch(e) {
            assert(false, "COW fails : "+e.message);
          }
        }, true);
        iframe.setAttribute("src", url);
        document.body.appendChild(iframe);
      });
    }
  );

  worker.port.on("end", function () {
    server.stop(helper.done);
  });

  worker.postMessage("http://localhost:" + serverPort + "/");

});


let html = '<a href="foo">link</a>';
exports["test MutationObvserver"] = createProxyTest(html, function (helper) {

  helper.createWorker(
    'new ' + function ContentScriptScope() {
      if (typeof MutationObserver == "undefined") {
        assert(true, "No MutationObserver for this FF version");
        done();
        return;
      }
      let link = document.getElementsByTagName("a")[0];

      
      let obs = new MutationObserver(function(mutations){
        
        assert(mutations.length == 1, "only one attribute mutation");
        let mutation = mutations[0];
        assert(mutation.type == "attributes", "check `type`");
        assert(mutation.target == link, "check `target`");
        assert(mutation.attributeName == "href", "check `attributeName`");
        assert(mutation.oldValue == "foo", "check `oldValue`");
        obs.disconnect();
        done();
      });
      obs.observe(document, {
        subtree: true,
        attributes: true,
        attributeOldValue: true,
        attributeFilter: ["href"]
      });

      
      link.setAttribute("href", "bar");
    }
  );

});

let html = '<script>' +
  'var accessCheck = function() {' +
  '  assert(true, "exporting function works");' +
  '  try{' +
  '    exportedObj.prop;' +
  '    assert(false, "content should not have access to content-script");' +
  '  } catch(e) {' +
  '    assert(e.toString().indexOf("Permission denied") != -1,' +
  '           "content should not have access to content-script");' +
  '  }' +
  '}</script>';
exports["test nsEp for content-script"] = createProxyTest(html, function (helper) {

  helper.createWorker(
    'let glob = this; new ' + function ContentScriptScope() {

      exportFunction(assert, unsafeWindow, { defineAs: "assert" });
      window.wrappedJSObject.assert(true, "assert exported");
      window.wrappedJSObject.exportedObj = { prop: 42 };
      window.wrappedJSObject.accessCheck();
      done();
    }
  );

});

require("sdk/test").run(exports);
