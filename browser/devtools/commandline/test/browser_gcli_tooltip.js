















'use strict';





var exports = {};

var TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testTooltip.js</p>";

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






exports.testActivate = function(options) {
  if (!options.display) {
    assert.log('No display. Skipping activate tests');
    return;
  }

  return helpers.audit(options, [
    {
      setup:    ' ',
      check: {
        input:  ' ',
        hints:   '',
        markup: 'V',
        cursor: 1,
        current: '__command',
        status: 'ERROR',
        message:  '',
        unassigned: [ ],
        outputState: 'false:default',
        tooltipState: 'false:default'
      }
    },
    {
      setup:    'tsb ',
      check: {
        input:  'tsb ',
        hints:      'false',
        markup: 'VVVV',
        cursor: 4,
        current: 'toggle',
        status: 'VALID',
        options: [ 'false', 'true' ],
        message:  '',
        predictions: [ 'false', 'true' ],
        unassigned: [ ],
        outputState: 'false:default',
        tooltipState: 'true:importantFieldFlag'
      }
    },
    {
      setup:    'tsb t',
      check: {
        input:  'tsb t',
        hints:       'rue',
        markup: 'VVVVI',
        cursor: 5,
        current: 'toggle',
        status: 'ERROR',
        options: [ 'true' ],
        message:  '',
        predictions: [ 'true' ],
        unassigned: [ ],
        outputState: 'false:default',
        tooltipState: 'true:importantFieldFlag'
      }
    },
    {
      setup:    'tsb tt',
      check: {
        input:  'tsb tt',
        hints:        ' -> true',
        markup: 'VVVVII',
        cursor: 6,
        current: 'toggle',
        status: 'ERROR',
        options: [ 'true' ],
        message: '',
        predictions: [ 'true' ],
        unassigned: [ ],
        outputState: 'false:default',
        tooltipState: 'true:importantFieldFlag'
      }
    },
    {
      setup:    'wxqy',
      check: {
        input:  'wxqy',
        hints:      '',
        markup: 'EEEE',
        cursor: 4,
        current: '__command',
        status: 'ERROR',
        options: [ ],
        message:  'Can\'t use \'wxqy\'.',
        predictions: [ ],
        unassigned: [ ],
        outputState: 'false:default',
        tooltipState: 'true:isError'
      }
    },
    {
      setup:    '',
      check: {
        input:  '',
        hints:  '',
        markup: '',
        cursor: 0,
        current: '__command',
        status: 'ERROR',
        message: '',
        unassigned: [ ],
        outputState: 'false:default',
        tooltipState: 'false:default'
      }
    }
  ]);
};
