



"use strict";

module.metadata = {
  "stability": "unstable"
};





const { Worker: WorkerTrait } = require("../content/worker");
const { Loader } = require("../content/loader");
const { merge } = require("../util/object");
const { emit } = require("../event/core");

const LegacyWorker = WorkerTrait.resolve({
  _setListeners: "__setListeners",
}).compose(Loader, {
  _setListeners: function() {},
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
