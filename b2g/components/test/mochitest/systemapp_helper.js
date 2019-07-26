const Cu = Components.utils;

const { Services } = Cu.import("resource://gre/modules/Services.jsm");


let scope = {};
Services.scriptloader.loadSubScript("resource://gre/modules/SystemAppProxy.jsm", scope);
const { SystemAppProxy } = scope;

let frame;
let customEventTarget;

let index = -1;
function next() {
  index++;
  if (index >= steps.length) {
    assert.ok(false, "Shouldn't get here!");
    return;
  }
  try {
    steps[index]();
  } catch(ex) {
    assert.ok(false, "Caught exception: " + ex);
  }
}



let isLoaded = false;
let n = 0;
function listener(event) {
  if (!isLoaded) {
    assert.ok(false, "Received event before the iframe is ready");
    return;
  }
  n++;
  if (n == 1) {
    assert.equal(event.type, "mozChromeEvent");
    assert.equal(event.detail.name, "first");
  } else if (n == 2) {
    assert.equal(event.type, "custom");
    assert.equal(event.detail.name, "second");

    next(); 
  } else if (n == 3) {
    assert.equal(event.type, "custom");
    assert.equal(event.detail.name, "third");
  } else if (n == 4) {
    assert.equal(event.type, "mozChromeEvent");
    assert.equal(event.detail.name, "fourth");
  } else if (n == 5) {
    assert.equal(event.type, "custom");
    assert.equal(event.detail.name, "fifth");
    assert.equal(event.target, customEventTarget);

    next(); 
  } else {
    assert.ok(false, "Unexpected event of type " + event.type);
  }
}


let steps = [
  function waitForWebapps() {
    
    
    let { DOMApplicationRegistry } =  Cu.import('resource://gre/modules/Webapps.jsm', {});
    DOMApplicationRegistry.registryReady.then(function () {
      next();
    });
  },

  function earlyEvents() {
    
    SystemAppProxy.dispatchEvent({ name: "first" });
    SystemAppProxy._sendCustomEvent("custom", { name: "second" });
    next();
  },

  function createFrame() {
    
    let win = Services.wm.getMostRecentWindow("navigator:browser");
    let doc = win.document;
    frame = doc.createElement("iframe");
    doc.documentElement.appendChild(frame);

    customEventTarget = frame.contentDocument.body;

    
    
    frame.contentWindow.addEventListener("mozChromeEvent", listener);
    frame.contentWindow.addEventListener("custom", listener);

    
    
    function removedListener() {
      assert(false, "Listener isn't correctly removed from the pending list");
    }
    SystemAppProxy.addEventListener("mozChromeEvent", removedListener);
    SystemAppProxy.removeEventListener("mozChromeEvent", removedListener);

    
    SystemAppProxy.registerFrame(frame);
    assert.ok(true, "Frame created and registered");

    frame.contentWindow.addEventListener("load", function onload() {
      frame.contentWindow.removeEventListener("load", onload);
      assert.ok(true, "Frame document loaded");

      
      
      isLoaded = true;
      SystemAppProxy.setIsReady();
      assert.ok(true, "Frame declared as loaded");

      
      
    });

    frame.setAttribute("src", "data:text/html,system app");
  },

  function checkEventDispatching() {
    
    
    SystemAppProxy._sendCustomEvent("custom", { name: "third" });
    SystemAppProxy.dispatchEvent({ name: "fourth" });
    SystemAppProxy._sendCustomEvent("custom", { name: "fifth" }, false, customEventTarget);
    
  },

  function checkEventListening() {
    SystemAppProxy.addEventListener("mozContentEvent", function onContentEvent(event) {
      assert.equal(event.detail.name, "first-content", "received a system app event");
      SystemAppProxy.removeEventListener("mozContentEvent", onContentEvent);

      next();
    });
    let win = frame.contentWindow;
    win.dispatchEvent(new win.CustomEvent("mozContentEvent", { detail: {name: "first-content"} }));
  },

  function endOfTest() {
    frame.remove();
    sendAsyncMessage("finish");
  }
];

next();
