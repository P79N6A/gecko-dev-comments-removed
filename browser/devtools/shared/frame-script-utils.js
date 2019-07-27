



"use strict";
const Cu = Components.utils;

const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
devtools.lazyImporter(this, "promise", "resource://gre/modules/Promise.jsm", "Promise");
devtools.lazyImporter(this, "Task", "resource://gre/modules/Task.jsm", "Task");

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
