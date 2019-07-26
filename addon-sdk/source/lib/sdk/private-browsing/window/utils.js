


'use strict';

module.metadata = {
  'stability': 'unstable'
};

const privateNS = require('../../core/namespace').ns();

function getOwnerWindow(thing) {
  try {
    
    let fn = (privateNS(thing.prototype) || privateNS(thing) || {}).getOwnerWindow;

    if (fn)
      return fn.apply(fn, [thing].concat(arguments));
  }
  
  catch(e) {}
  
  return undefined;
}
getOwnerWindow.define = function(Type, fn) {
  privateNS(Type.prototype).getOwnerWindow = fn;
}

getOwnerWindow.implement = function(instance, fn) {
  privateNS(instance).getOwnerWindow = fn;
}

exports.getOwnerWindow = getOwnerWindow;
