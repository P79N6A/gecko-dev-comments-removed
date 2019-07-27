



"use strict";

const { on, once, off, emit, count } = require("sdk/event/core");

const { setImmediate, setTimeout } = require("sdk/timers");
const { defer } = require("sdk/core/promise");


















const wait = (target, type, capture) => {
  let { promise, resolve, reject } = defer();

  if (!arguments.length) {
    setImmediate(resolve);
  }
  else if (typeof(target) === "number") {
    setTimeout(resolve, target);
  }
  else if (typeof(target.once) === "function") {
    target.once(type, resolve);
  }
  else if (typeof(target.addEventListener) === "function") {
    target.addEventListener(type, function listener(...args) {
      this.removeEventListener(type, listener, capture);
      resolve(...args);
    }, capture);
  }
  else if (typeof(target) === "object" && target !== null) {
    once(target, type, resolve);
  }
  else {
    reject('Invalid target given.');
  }

  return promise;
};
exports.wait = wait;

function scenario(setup) {
  return function(unit) {
    return function(assert) {
      let actual = [];
      let input = {};
      unit(input, function(output, events, expected, message) {
        let result = setup(output, expected, actual);

        events.forEach(function(event) emit(input, "data", event));

        assert.deepEqual(actual, result, message);
      });
    }
  }
}

exports.emits = scenario(function(output, expected, actual) {
  on(output, "data", function(data) actual.push(this, data));

  return expected.reduce(function($$, $) $$.concat(output, $), []);
});

exports.registerOnce = scenario(function(output, expected, actual) {
  function listener(data) actual.push(data);
  on(output, "data", listener);
  on(output, "data", listener);
  on(output, "data", listener);

  return expected;
});

exports.ignoreNew = scenario(function(output, expected, actual) {
  on(output, "data", function(data) {
    actual.push(data + "#1");
    on(output, "data", function(data) {
      actual.push(data + "#2");
    });
  });

  return expected.map(function($) $ + "#1");
});

exports.FIFO = scenario(function(target, expected, actual) {
  on(target, "data", function($) actual.push($ + "#1"));
  on(target, "data", function($) actual.push($ + "#2"));
  on(target, "data", function($) actual.push($ + "#3"));

  return expected.reduce(function(result, value) {
    return result.concat(value + "#1", value + "#2", value + "#3");
  }, []);
});