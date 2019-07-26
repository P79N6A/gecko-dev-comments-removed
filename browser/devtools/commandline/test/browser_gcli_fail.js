






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testFail.js</p>";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.runTests(options, exports);
  }).then(finish);
}



'use strict';




exports.setup = function(options) {
  mockCommands.setup();
};

exports.shutdown = function(options) {
  mockCommands.shutdown();
};

exports.testBasic = function(options) {
  return helpers.audit(options, [
    {
      setup: 'tsfail reject',
      exec: {
        completed: false,
        output: 'rejected promise',
        type: 'error',
        error: true
      }
    },
    {
      setup: 'tsfail rejecttyped',
      exec: {
        completed: false,
        output: '54',
        type: 'number',
        error: true
      }
    },
    {
      setup: 'tsfail throwerror',
      exec: {
        completed: true,
        output: 'Error: thrown error',
        type: 'error',
        error: true
      }
    },
    {
      setup: 'tsfail throwstring',
      exec: {
        completed: true,
        output: 'thrown string',
        type: 'error',
        error: true
      }
    },
    {
      setup: 'tsfail noerror',
      exec: {
        completed: true,
        output: 'no error',
        type: 'string',
        error: false
      }
    }
  ]);
};



