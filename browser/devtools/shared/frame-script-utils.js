



"use strict";
const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;
const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
devtools.lazyImporter(this, "promise", "resource://gre/modules/Promise.jsm", "Promise");
devtools.lazyImporter(this, "Task", "resource://gre/modules/Task.jsm", "Task");
const loader = Cc["@mozilla.org/moz/jssubscript-loader;1"]
            .getService(Ci.mozIJSSubScriptLoader);
let EventUtils = {};
loader.loadSubScript("chrome://marionette/content/EventUtils.js", EventUtils);

addMessageListener("devtools:test:history", function ({ data }) {
  content.history[data.direction]();
});

addMessageListener("devtools:test:navigate", function ({ data }) {
  content.location = data.location;
});

addMessageListener("devtools:test:reload", function ({ data }) {
  data = data || {};
  content.location.reload(data.forceget);
});

addMessageListener("devtools:test:console", function ({ data }) {
  let method = data.shift();
  content.console[method].apply(content.console, data);
});

















function promiseXHR(data) {
  let xhr = new content.XMLHttpRequest();

  let method = data.method || "GET";
  let url = data.url || content.location.href;
  let body = data.body || "";

  if (data.nocache) {
    url += "?devtools-cachebust=" + Math.random();
  }

  let deferred = promise.defer();
  xhr.addEventListener("loadend", function loadend(event) {
    xhr.removeEventListener("loadend", loadend);
    deferred.resolve({ status: xhr.status, response: xhr.response });
  });

  xhr.open(method, url);
  xhr.send(body);
  return deferred.promise;

}























addMessageListener("devtools:test:xhr", Task.async(function* ({ data }) {
  let requests = Array.isArray(data) ? data : [data];
  let responses = [];

  for (let request of requests) {
    let response = yield promiseXHR(request);
    responses.push(response);
  }

  sendAsyncMessage("devtools:test:xhr", responses);
}));



addMessageListener("devtools:test:eval", function ({ data }) {
  sendAsyncMessage("devtools:test:eval:response", {
    value: content.eval(data.script),
    id: data.id
  });
});

addEventListener("load", function() {
  sendAsyncMessage("devtools:test:load");
}, true);









addMessageListener("devtools:test:setStyle", function(msg) {
  let {selector, propertyName, propertyValue} = msg.data;
  let node = superQuerySelector(selector);
  if (!node) {
    return;
  }

  node.style[propertyName] = propertyValue;

  sendAsyncMessage("devtools:test:setStyle");
});















addMessageListener("devtools:test:getDomElementInfo", function(msg) {
  let {selector} = msg.data;
  let node = superQuerySelector(selector);

  let info = null;
  if (node) {
    info = {
      tagName: node.tagName,
      namespaceURI: node.namespaceURI,
      numChildren: node.children.length,
      attributes: [...node.attributes].map(({name, value, namespaceURI}) => {
        return {name, value, namespaceURI};
      }),
      outerHTML: node.outerHTML,
      innerHTML: node.innerHTML,
      textContent: node.textContent
    };
  }

  sendAsyncMessage("devtools:test:getDomElementInfo", info);
});









addMessageListener("devtools:test:setAttribute", function(msg) {
  let {selector, attributeName, attributeValue} = msg.data;
  let node = superQuerySelector(selector);
  if (!node) {
    return;
  }

  node.setAttribute(attributeName, attributeValue);

  sendAsyncMessage("devtools:test:setAttribute");
});
















addMessageListener("Test:SynthesizeMouse", function(msg) {
  let {x, y, center, options, selector} = msg.data;
  let {node} = msg.objects;

  if (!node && selector) {
    node = superQuerySelector(selector);
  }

  if (center) {
    EventUtils.synthesizeMouseAtCenter(node, options, node.ownerDocument.defaultView);
  } else {
    EventUtils.synthesizeMouse(node, x, y, options, node.ownerDocument.defaultView);
  }

  
  
  
  sendAsyncMessage("Test:SynthesizeMouse");
});









addMessageListener("Test:SynthesizeKey", function(msg) {
  let {key, options} = msg.data;

  EventUtils.synthesizeKey(key, options, content);
});











function superQuerySelector(superSelector, root=content.document) {
  let frameIndex = superSelector.indexOf("||");
  if (frameIndex === -1) {
    return root.querySelector(superSelector);
  } else {
    let rootSelector = superSelector.substring(0, frameIndex).trim();
    let childSelector = superSelector.substring(frameIndex+2).trim();
    root = root.querySelector(rootSelector);
    if (!root || !root.contentWindow) {
      return null;
    }

    return superQuerySelector(childSelector, root.contentWindow.document);
  }
}
