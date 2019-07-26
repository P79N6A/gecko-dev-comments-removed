






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testMenu.js</p>";

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

exports.testOptions = function(options) {
  return helpers.audit(options, [
    {
      setup:    'tslong',
      check: {
        input:  'tslong',
        markup: 'VVVVVV',
        status: 'ERROR',
        hints: ' <msg> [options]',
        args: {
          msg: { value: undefined, status: 'INCOMPLETE' },
          num: { value: undefined, status: 'VALID' },
          sel: { value: undefined, status: 'VALID' },
          bool: { value: false, status: 'VALID' },
          bool2: { value: false, status: 'VALID' },
          sel2: { value: undefined, status: 'VALID' },
          num2: { value: undefined, status: 'VALID' }
        }
      }
    }
  ]);
};




