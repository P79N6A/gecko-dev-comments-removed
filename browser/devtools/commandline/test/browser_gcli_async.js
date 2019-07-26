















'use strict';





var exports = {};

var TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testAsync.js</p>";

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
      setup:    'tsslo',
      check: {
        input:  'tsslo',
        hints:       'w',
        markup: 'IIIII',
        cursor: 5,
        current: '__command',
        status: 'ERROR',
        predictions: ['tsslow'],
        unassigned: [ ]
      }
    },
    {
      setup:    'tsslo<TAB>',
      check: {
        input:  'tsslow ',
        hints:         'Shalom',
        markup: 'VVVVVVV',
        cursor: 7,
        current: 'hello',
        status: 'ERROR',
        predictions: [
          'Shalom', 'Namasté', 'Hallo', 'Dydd-da', 'Chào', 'Hej',
          'Saluton', 'Sawubona'
        ],
        unassigned: [ ],
        args: {
          command: { name: 'tsslow' },
          hello: {
            value: undefined,
            arg: '',
            status: 'INCOMPLETE'
          },
        }
      }
    },
    {
      setup:    'tsslow S',
      check: {
        input:  'tsslow S',
        hints:          'halom',
        markup: 'VVVVVVVI',
        cursor: 8,
        current: 'hello',
        status: 'ERROR',
        predictions: [ 'Shalom', 'Saluton', 'Sawubona', 'Namasté' ],
        unassigned: [ ],
        args: {
          command: { name: 'tsslow' },
          hello: {
            value: undefined,
            arg: ' S',
            status: 'INCOMPLETE'
          },
        }
      }
    },
    {
      setup:    'tsslow S<TAB>',
      check: {
        input:  'tsslow Shalom ',
        hints:                '',
        markup: 'VVVVVVVVVVVVVV',
        cursor: 14,
        current: 'hello',
        status: 'VALID',
        predictions: [ 'Shalom' ],
        unassigned: [ ],
        args: {
          command: { name: 'tsslow' },
          hello: {
            value: 'Shalom',
            arg: ' Shalom ',
            status: 'VALID',
            message: ''
          },
        }
      }
    }
  ]);
};
