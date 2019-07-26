


"use strict";

const memory = require("sdk/deprecated/memory");
const { gc } = require("sdk/test/memory");

exports.testMemory = function(assert) {
  var obj = {};
  memory.track(obj, "testMemory.testObj");

  var objs = memory.getObjects("testMemory.testObj");
  assert.equal(objs[0].weakref.get(), obj);
  obj = null;

  gc().then(function() {
    assert.equal(objs[0].weakref.get(), null);
  });
};

require('sdk/test').run(exports);
