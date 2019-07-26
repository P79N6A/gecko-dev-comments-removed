



var memory = require("sdk/deprecated/memory");

exports.testMemory = function(assert) {
  assert.pass("Skipping this test until Gecko memory debugging issues " +
            "are resolved (see bug 592774).");
  return;

  var obj = {};
  memory.track(obj, "testMemory.testObj");
  var objs = memory.getObjects("testMemory.testObj");
  assert.equal(objs[0].weakref.get(), obj);
  obj = null;
  memory.gc();
  assert.equal(objs[0].weakref.get(), null);
};

require('sdk/test').run(exports);
