







"use strict";

module.metadata = {
  "stability": "unstable"
};

const { deprecateFunction } = require("../util/deprecate");
const { setImmediate, setTimeout, clearTimeout } = require("../timers");

const arity = f => f.arity || f.length;

const name = f => f.displayName || f.name;

const derive = (f, source) => {
  f.displayName = name(source);
  f.arity = arity(source);
  return f;
};








const method = (...lambdas) => {
  return function method(...args) {
    args.unshift(this);
    return lambdas.reduce((_, lambda) => lambda.apply(this, args),
                          void(0));
  };
};
exports.method = method;








const defer = f => derive(function(...args) {
  setImmediate(invoke, f, args, this);
}, f);
exports.defer = defer;

exports.remit = defer;











const invoke = (callee, params, self) => callee.apply(self, params);
exports.invoke = invoke;










const partial = (f, ...curried) => {
  if (typeof(f) !== "function")
    throw new TypeError(String(f) + " is not a function");

  let fn = derive(function(...args) {
    return f.apply(this, curried.concat(args));
  }, f);
  fn.arity = arity(f) - curried.length;
  return fn;
};
exports.partial = partial;















const curry = new function() {
  const currier = (fn, arity, params) => {
    
    
    const curried = function(...input) {
      
      if (params) input.unshift.apply(input, params);
      
      
      return (input.length >= arity) ? fn.apply(this, input) :
             currier(fn, arity, input);
    };
    curried.arity = arity - (params ? params.length : 0);

    return curried;
  };

  return fn => currier(fn, arity(fn));
};
exports.curry = curry;













function compose(...lambdas) {
  return function composed(...args) {
    let index = lambdas.length;
    while (0 <= --index)
      args = [lambdas[index].apply(this, args)];

    return args[0];
  };
}
exports.compose = compose;














const wrap = (f, wrapper) => derive(function wrapped(...args) {
  return wrapper.apply(this, [f].concat(args));
}, f);
exports.wrap = wrap;




const identity = value => value;
exports.identity = identity;








const memoize = (f, hasher) => {
  let memo = Object.create(null);
  let cache = new WeakMap();
  hasher = hasher || identity;
  return derive(function memoizer(...args) {
    const key = hasher.apply(this, args);
    const type = typeof(key);
    if (key && (type === "object" || type === "function")) {
      if (!cache.has(key))
        cache.set(key, f.apply(this, args));
      return cache.get(key);
    }
    else {
      if (!(key in memo))
        memo[key] = f.apply(this, args);
      return memo[key];
    }
  }, f);
};
exports.memoize = memoize;






const delay = function delay(f, ms, ...args) {
  setTimeout(() => f.apply(this, args), ms);
};
exports.delay = delay;







const once = f => {
  let ran = false, cache;
  return derive(function(...args) {
    return ran ? cache : (ran = true, cache = f.apply(this, args));
  }, f);
};
exports.once = once;

exports.cache = once;




const complement = f => derive(function(...args) {
  return args.length < arity(f) ? complement(partial(f, ...args)) :
         !f.apply(this, args);
}, f);
exports.complement = complement;



const constant = x => _ => x;
exports.constant = constant;






const when = (p, consequent, alternate) => {
  if (typeof(alternate) !== "function" && alternate !== void(0))
    throw TypeError("alternate must be a function");
  if (typeof(consequent) !== "function")
    throw TypeError("consequent must be a function");

  return function(...args) {
    return p.apply(this, args) ?
           consequent.apply(this, args) :
           alternate && alternate.apply(this, args);
  };
};
exports.when = when;




const apply = (f, ...rest) => f.apply(f, rest.concat(rest.pop()));
exports.apply = apply;



const flip = f => derive(function(...args) {
  return f.apply(this, args.reverse());
}, f);
exports.flip = flip;







const field = curry((name, target) =>
  
  target == null ? target : target[name]);
exports.field = field;





const query = curry((path, target) => {
  const names = path.split(".");
  const count = names.length;
  let index = 0;
  let result = target;
  
  while (result != null && index < count) {
    result = result[names[index]];
    index = index + 1;
  }
  return result;
});
exports.query = query;





const isInstance = curry((Type, value) => value instanceof Type);
exports.isInstance = isInstance;




const chainable = f => derive(function(...args) {
  f.apply(this, args);
  return this;
}, f);
exports.chainable = chainable;
exports.chain =
  deprecateFunction(chainable, "Function `chain` was renamed to `chainable`");






const is = curry((expected, actual) => actual === expected);
exports.is = is;

const isnt = complement(is);
exports.isnt = isnt;







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
