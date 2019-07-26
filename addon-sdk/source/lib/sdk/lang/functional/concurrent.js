







"use strict";

module.metadata = {
  "stability": "unstable"
};

const { arity, name, derive, invoke } = require("./helpers");
const { setTimeout, clearTimeout, setImmediate } = require("../../timers");








const defer = f => derive(function(...args) {
  setImmediate(invoke, f, args, this);
}, f);
exports.defer = defer;

exports.remit = defer;






const delay = function delay(f, ms, ...args) {
  setTimeout(() => f.apply(this, args), ms);
};
exports.delay = delay;







const debounce = function debounce (fn, wait) {
  let timeout, args, context, timestamp, result;

  let later = function () {
    let last = Date.now() - timestamp;
    if (last < wait) {
      timeout = setTimeout(later, wait - last);
    } else {
      timeout = null;
      result = fn.apply(context, args);
      context = args = null;
    }
  };

  return function (...aArgs) {
    context = this;
    args = aArgs;
    timestamp  = Date.now();
    if (!timeout) {
      timeout = setTimeout(later, wait);
    }

    return result;
  };
};
exports.debounce = debounce;







const throttle = function throttle (func, wait, options) {
  let context, args, result;
  let timeout = null;
  let previous = 0;
  options || (options = {});
  let later = function() {
    previous = options.leading === false ? 0 : Date.now();
    timeout = null;
    result = func.apply(context, args);
    context = args = null;
  };
  return function() {
    let now = Date.now();
    if (!previous && options.leading === false) previous = now;
    let remaining = wait - (now - previous);
    context = this;
    args = arguments;
    if (remaining <= 0) {
      clearTimeout(timeout);
      timeout = null;
      previous = now;
      result = func.apply(context, args);
      context = args = null;
    } else if (!timeout && options.trailing !== false) {
      timeout = setTimeout(later, remaining);
    }
    return result;
  };
};
exports.throttle = throttle;
