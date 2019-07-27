


'use strict';

const traceback = require('sdk/console/traceback');
const REQUIRE_LINE_NO = 29;

exports.test_no_args = function(assert) {
  let passed = tryRequireModule(assert);
  assert.ok(passed, 'require() with no args should raise helpful error');
};

exports.test_invalid_sdk_module = function (assert) {
  let passed = tryRequireModule(assert, 'sdk/does-not-exist');
  assert.ok(passed, 'require() with an invalid sdk module should raise');
};

exports.test_invalid_relative_module = function (assert) {
  let passed = tryRequireModule(assert, './does-not-exist');
  assert.ok(passed, 'require() with an invalid relative module should raise');
};


function tryRequireModule(assert, module) {
  let passed = false;
  try {
    
    let doesNotExist = require(module);
  }
  catch(e) {
    checkError(assert, module, e);
    passed = true;
  }
  return passed;
}

function checkError (assert, name, e) {
  let msg = e.toString();
  if (name) {
    assert.ok(/is not found at/.test(msg),
      'Error message indicates module not found');
    assert.ok(msg.indexOf(name.replace(/\./g,'')) > -1,
      'Error message has the invalid module name in the message');
  }
  else {
    assert.equal(msg.indexOf('Error: you must provide a module name when calling require() from '), 0);
    assert.ok(msg.indexOf("test-require") !== -1, msg);
  }

  
  
  let tb = traceback.fromException(e);

  
  let lastFrame = tb[tb.length - 1];
  if (lastFrame.fileName.indexOf("toolkit/loader.js") !== -1 ||
      lastFrame.fileName.indexOf("sdk/loader/cuddlefish.js") !== -1)
    lastFrame = tb[tb.length - 2];

  assert.ok(lastFrame.fileName.indexOf("test-require.js") !== -1,
                          'Filename found in stacktrace');
  assert.equal(lastFrame.lineNumber, REQUIRE_LINE_NO,
                          'stacktrace has correct line number');
}

require('sdk/test').run(exports);
