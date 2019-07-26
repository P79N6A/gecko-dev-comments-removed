







"use strict";

module.metadata = {
  "stability": "unstable"
};

const { defer, remit, delay, debounce,
        throttle } = require("./functional/concurrent");
const { method, invoke, partial, curry, compose, wrap, identity, memoize, once,
        cache, complement, constant, when, apply, flip, field, query,
        isInstance, chainable, is, isnt } = require("./functional/core");

exports.defer = defer;
exports.remit = remit;
exports.delay = delay;
exports.debounce = debounce;
exports.throttle = throttle;

exports.method = method;
exports.invoke = invoke;
exports.partial = partial;
exports.curry = curry;
exports.compose = compose;
exports.wrap = wrap;
exports.identity = identity;
exports.memoize = memoize;
exports.once = once;
exports.cache = cache;
exports.complement = complement;
exports.constant = constant;
exports.when = when;
exports.apply = apply;
exports.flip = flip;
exports.field = field;
exports.query = query;
exports.isInstance = isInstance;
exports.chainable = chainable;
exports.is = is;
exports.isnt = isnt;
