





"use strict";

module.metadata = {
  "stability": "unstable"
};

const UNCAUGHT_ERROR = 'An error event was emitted for which there was no listener.';
const BAD_LISTENER = 'The event listener must be a function.';

const { ns } = require('../core/namespace');

const event = ns();




const observers = function observers(target, type) {
  let listeners = event(target);
  return type in listeners ? listeners[type] : listeners[type] = [];
};











function on(target, type, listener) {
   if (typeof(listener) !== 'function')
    throw new Error(BAD_LISTENER);

  let listeners = observers(target, type);
  if (!~listeners.indexOf(listener))
    listeners.push(listener);
}
exports.on = on;











function once(target, type, listener) {
  on(target, type, function observer() {
    off(target, type, observer);
    listener.apply(target, arguments);
  });
}
exports.once = once;

















function emit(target, type, message ) {
  for each (let item in emit.lazy.apply(emit.lazy, arguments)) {
    
  }
}









emit.lazy = function lazy(target, type, message ) {
  let args = Array.slice(arguments, 2);
  let listeners = observers(target, type).slice();
  let index = 0;
  let count = listeners.length;

  
  
  if (count === 0 && type === 'error') console.exception(message);
  while (index < count) {
    try { yield listeners[index].apply(target, args); }
    catch (error) {
      
      
      if (type !== 'error') emit(target, 'error', error);
      else console.exception(error);
    }
    index = index + 1;
  }
}
exports.emit = emit;













function off(target, type, listener) {
  let length = arguments.length;
  if (length === 3) {
    let listeners = observers(target, type);
    let index = listeners.indexOf(listener);
    if (~index)
      listeners.splice(index, 1);
  }
  else if (length === 2) {
    observers(target, type).splice(0);
  }
  else if (length === 1) {
    let listeners = event(target);
    Object.keys(listeners).forEach(function(type) delete listeners[type]);
  }
}
exports.off = off;





function count(target, type) {
  return observers(target, type).length;
}
exports.count = count;
