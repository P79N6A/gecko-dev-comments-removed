






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testFocus.js</p>";

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
      skipRemainingIf: options.isJsdom,
      name: 'exec setup',
      setup: function() {
        
        helpers.focusInput(options);
        return helpers.setInput(options, 'help');
      },
      check: { },
      exec: { }
    },
    {
      setup:    'tsn deep',
      check: {
        input:  'tsn deep',
        hints:          '',
        markup: 'IIIVIIII',
        cursor: 8,
        status: 'ERROR',
        outputState: 'false:default',
        tooltipState: 'false:default'
      }
    },
    {
      setup:    'tsn deep<TAB><RETURN>',
      check: {
        input:  'tsn deep ',
        hints:           '',
        markup: 'IIIIIIIIV',
        cursor: 9,
        status: 'ERROR',
        outputState: 'false:default',
        tooltipState: 'true:isError'
      }
    }
  ]);
};



