


'use strict';

module.metadata = {
  'stability': 'experimental'
};

const method = require('../../method/core');
const { uuid } = require('../util/uuid');


function memoize(f) {
  const memo = new WeakMap();

  return function memoizer(o) {
    let key = o;
    if (!memo.has(key))
      memo.set(key, f.apply(this, arguments));
    return memo.get(key);
  };
}

let identify = method('identify');
identify.define(Object, memoize(function() { return uuid(); }));
exports.identify = identify;
