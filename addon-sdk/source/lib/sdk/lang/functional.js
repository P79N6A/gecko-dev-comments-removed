







"use strict";

module.metadata = {
  "stability": "unstable"
};

const { setTimeout } = require("../timers");
const { deprecateFunction } = require("../util/deprecate");






function method(lambda) {
  return function method() {
    return lambda.apply(null, [this].concat(Array.slice(arguments)));
  }
}
exports.method = method;








function defer(f) {
  return function deferred()
    setTimeout(invoke, 0, f, arguments, this);
}
exports.defer = defer;

exports.remit = defer;




function chain(f) {
  return function chainable(...args) {
    f.apply(this, args);
    return this;
  };
}
exports.chain = chain;











function invoke(callee, params, self) callee.apply(self, params);
exports.invoke = invoke;










function partial(fn) {
  if (typeof fn !== "function")
    throw new TypeError(String(fn) + " is not a function");

  let args = Array.slice(arguments, 1);

  return function() fn.apply(this, args.concat(Array.slice(arguments)));
}
exports.partial = partial;















var curry = new function() {
  function currier(fn, arity, params) {
    
    
    return function curried() {
      var input = Array.slice(arguments);
      
      if (params) input.unshift.apply(input, params);
      
      
      return (input.length >= arity) ? fn.apply(this, input) :
             currier(fn, arity, input);
    };
  }

  return function curry(fn) {
    return currier(fn, fn.length);
  }
};
exports.curry = curry;













function compose() {
  let lambdas = Array.slice(arguments);
  return function composed() {
    let args = Array.slice(arguments), index = lambdas.length;
    while (0 <= --index)
      args = [ lambdas[index].apply(this, args) ];
    return args[0];
  };
}
exports.compose = compose;














function wrap(f, wrapper) {
  return function wrapped()
    wrapper.apply(this, [ f ].concat(Array.slice(arguments)))
};
exports.wrap = wrap;




function identity(value) value
exports.identity = identity;








function memoize(f, hasher) {
  let memo = Object.create(null);
  hasher = hasher || identity;
  return function memoizer() {
    let key = hasher.apply(this, arguments);
    return key in memo ? memo[key] : (memo[key] = f.apply(this, arguments));
  };
}
exports.memoize = memoize;






function delay(f, ms) {
  let args = Array.slice(arguments, 2);
  setTimeout(function(context) { return f.apply(context, args); }, ms, this);
};
exports.delay = delay;







function once(f) {
  let ran = false, cache;
  return function() ran ? cache : (ran = true, cache = f.apply(this, arguments))
};
exports.once = once;

exports.cache = once;
