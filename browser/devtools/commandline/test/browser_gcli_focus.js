















'use strict';





var exports = {};

var TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testFocus.js</p>";

function test() {
  return Task.spawn(function() {
    let options = yield helpers.openTab(TEST_URI);
    yield helpers.openToolbar(options);
    gcli.addItems(mockCommands.items);

    yield helpers.runTests(options, exports);

    gcli.removeItems(mockCommands.items);
    yield helpers.closeToolbar(options);
    yield helpers.closeTab(options);
  }).then(finish, helpers.handleError);
}





exports.testBasic = function(options) {
  return helpers.audit(options, [
    {
      name: 'exec setup',
      setup: function() {
        
        helpers.focusInput(options);
        return helpers.setInput(options, 'echo hi');
      },
      check: { },
      exec: { }
    },
    {
      setup:    'tsn deep',
      check: {
        input:  'tsn deep',
        hints:          ' down nested cmd',
        markup: 'IIIVIIII',
        cursor: 8,
        status: 'ERROR',
        outputState: 'false:default',
        tooltipState: 'false:default'
      }
    },
    {
      setup:    'tsn deep<TAB>',
      check: {
        input:  'tsn deep down nested cmd ',
        hints:                           '',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVVVV',
        cursor: 25,
        status: 'VALID',
        outputState: 'false:default',
        tooltipState: 'false:default'
      }
    }
  ]);
};
