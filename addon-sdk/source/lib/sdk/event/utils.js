


"use strict";

module.metadata = {
  "stability": "unstable"
};

let { emit, on, off } = require("./core");









let refs = (function() {
  let refSets = new WeakMap();
  return function refs(target) {
    if (!refSets.has(target)) refSets.set(target, new Set());
    return refSets.get(target);
  }
})();

function transform(f, input) {
  let output = {};

  
  
  
  refs(output).add(input);

  function next(data) emit(output, "data", data);
  on(input, "error", function(error) emit(output, "error", error));
  on(input, "end", function() {
    refs(output).delete(input);
    emit(output, "end");
  });
  on(input, "data", function(data) f(data, next));
  return output;
}




function filter(predicate, input) {
  return transform(function(data, next) {
    if (predicate(data)) next(data)
  }, input);
}
exports.filter = filter;



function map(f, input) transform(function(data, next) next(f(data)), input)
exports.map = map;




function merge(inputs) {
  let output = {};
  let open = 1;
  let state = [];
  output.state = state;
  refs(output).add(inputs);

  function end(input) {
    open = open - 1;
    refs(output).delete(input);
    if (open === 0) emit(output, "end");
  }
  function error(e) emit(output, "error", e);
  function forward(input) {
    state.push(input);
    open = open + 1;
    on(input, "end", function() end(input));
    on(input, "error", error);
    on(input, "data", function(data) emit(output, "data", data));
  }

  
  if (Array.isArray(inputs)) {
    inputs.forEach(forward)
    end(inputs)
  }
  else {
    on(inputs, "end", function() end(inputs));
    on(inputs, "error", error);
    on(inputs, "data", forward);
  }

  return output;
}
exports.merge = merge;

function expand(f, inputs) merge(map(f, inputs))
exports.expand = expand;

function pipe(from, to) on(from, "*", emit.bind(emit, to))
exports.pipe = pipe;
