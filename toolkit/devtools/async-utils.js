



"use strict";










let {Cu} = require("chrome");
let {Task} = require("resource://gre/modules/Task.jsm");
let {Promise} = require("resource://gre/modules/Promise.jsm");









exports.async = function async(func) {
  return function(...args) {
    return Task.spawn(func.apply(this, args));
  };
};











exports.asyncOnce = function asyncOnce(func) {
  const promises = new WeakMap();
  return function(...args) {
    let promise = promises.get(this);
    if (!promise) {
      promise = Task.spawn(func.apply(this, args));
      promises.set(this, promise);
    }
    return promise;
  };
};














exports.listenOnce = function listenOnce(element, event, useCapture) {
  return new Promise(function(resolve, reject) {
    var onEvent = function(ev) {
      element.removeEventListener(event, onEvent, useCapture);
      resolve(ev);
    }
    element.addEventListener(event, onEvent, useCapture);
  });
};


















function promisify(obj, func, args) {
  return new Promise(resolve => {
    args.push((...results) => {
      resolve(results.length > 1 ? results : results[0]);
    });
    func.apply(obj, args);
  });
}







exports.promiseInvoke = function promiseInvoke(obj, func, ...args) {
  return promisify(obj, func, args);
};






exports.promiseCall = function promiseCall(func, ...args) {
  return promisify(undefined, func, args);
};
