















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_async.js");
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
            arg: ' Shalom ',
            status: 'VALID',
            message: ''
          },
        }
      }
    }
  ]);
};
