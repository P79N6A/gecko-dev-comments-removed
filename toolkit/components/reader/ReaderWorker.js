



"use strict";





importScripts("resource://gre/modules/workers/require.js",
	          "resource://gre/modules/reader/JSDOMParser.js",
	          "resource://gre/modules/reader/Readability.js");

let PromiseWorker = require("resource://gre/modules/workers/PromiseWorker.js");

let worker = new PromiseWorker.AbstractWorker();
worker.dispatch = function(method, args = []) {
  return Agent[method](...args);
};
worker.postMessage = function(result, ...transfers) {
  self.postMessage(result, ...transfers);
};
worker.close = function() {
  self.close();
};
worker.log = function(...args) {
  dump("ReaderWorker: " + args.join(" ") + "\n");
};

self.addEventListener("message", msg => worker.handleMessage(msg));

let Agent = {
  







  parseDocument: function (uri, serializedDoc) {
    let doc = new JSDOMParser().parse(serializedDoc);
    return new Readability(uri, doc).parse();
  },
};
