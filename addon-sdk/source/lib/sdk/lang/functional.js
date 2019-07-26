







"use strict";

module.metadata = {
  "stability": "unstable"
};

const { setTimeout } = require("../timers");






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











function invoke(callee, params, self) callee.apply(self, params);
exports.invoke = invoke;









function curry(fn) {
  if (typeof fn !== "function")
    throw new TypeError(String(fn) + " is not a function");

  let args = Array.slice(arguments, 1);

  return function() fn.apply(this, args.concat(Array.slice(arguments)));
}
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
