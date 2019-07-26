



"use strict";

module.metadata = {
  "stability": "unstable"
};





const { Worker: WorkerTrait } = require("../content/worker");
const { Loader } = require("../content/loader");
const { merge } = require("../util/object");
const { emit } = require("../event/core");

let assetsURI = require("../self").data.url();
let isArray = Array.isArray;

function isAddonContent({ contentURL }) {
  return typeof(contentURL) === "string" && contentURL.indexOf(assetsURI) === 0;
}

function hasContentScript({ contentScript, contentScriptFile }) {
  return (isArray(contentScript) ? contentScript.length > 0 :
         !!contentScript) ||
         (isArray(contentScriptFile) ? contentScriptFile.length > 0 :
         !!contentScriptFile);
}

function requiresAddonGlobal(model) {
  return isAddonContent(model) && !hasContentScript(model);
}
exports.requiresAddonGlobal = requiresAddonGlobal;


const LegacyWorker = WorkerTrait.compose(Loader).resolve({
  _setListeners: "__setListeners",
  _injectInDocument: "__injectInDocument",
  contentURL: "__contentURL"
}).compose({
  _setListeners: function() {},
  get contentURL() this._window.document.URL,
  get _injectInDocument() requiresAddonGlobal(this),
  attach: function(window) this._attach(window),
  detach: function() this._workerCleanup()
});



let traits = new WeakMap();

function traitFor(worker) traits.get(worker, null);

function WorkerHost(workerFor) {
  
  return ["postMessage", "port", "url", "tab"].reduce(function(proto, name) {
    Object.defineProperty(proto, name, {
      enumerable: true,
      configurable: false,
      get: function() traitFor(workerFor(this))[name],
      set: function(value) traitFor(workerFor(this))[name] = value
    });
    return proto;
  }, {});
}
exports.WorkerHost = WorkerHost;


function Worker(options) {
  let worker = Object.create(Worker.prototype);
  let trait = new LegacyWorker(options);
  ["pageshow", "pagehide", "detach", "message", "error"].forEach(function(key) {
    trait.on(key, function() {
      emit.apply(emit, [worker, key].concat(Array.slice(arguments)));
      
      
      emit.apply(emit, [worker, "*", key].concat(Array.slice(arguments)));
    });
  });
  traits.set(worker, trait);
  return worker;
}
exports.Worker = Worker;

function detach(worker) {
  let trait = traitFor(worker);
  if (trait) trait.detach();
}
exports.detach = detach;

function attach(worker, window) {
  let trait = traitFor(worker);
  
  trait.attach(window);
}
exports.attach = attach;

function destroy(worker) {
  let trait = traitFor(worker);
  if (trait) trait.destroy();
}
exports.destroy = destroy;
