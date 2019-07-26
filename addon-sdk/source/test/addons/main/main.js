


'use strict';

const { setTimeout } = require('sdk/timers');

let mainStarted = false;

exports.main = function main(options, callbacks) {
  mainStarted = true;

  let tests = {};

  tests.testMainArguments = function(assert) {
  	assert.ok(!!options, 'options argument provided to main');
    assert.ok('loadReason' in options, 'loadReason is in options provided by main');
    assert.equal(typeof callbacks.print, 'function', 'callbacks.print is a function');
    assert.equal(typeof callbacks.quit, 'function', 'callbacks.quit is a function');
    assert.equal(options.loadReason, 'install', 'options.loadReason is install');
  }

  require('sdk/test/runner').runTestsFromModule({exports: tests});
}


setTimeout(function() {
  if (mainStarted)
  	return;

  
  require("sdk/test/runner").runTestsFromModule({exports: {
  	testFail: function(assert) assert.fail('Main did not start..')
  }});
}, 500);
