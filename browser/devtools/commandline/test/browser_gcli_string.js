






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testString.js</p>";

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

exports.testNewLine = function(options) {
  helpers.audit(options, [
    {
      setup:    'echo a\\nb',
      check: {
        input:  'echo a\\nb',
        hints:            '',
        markup: 'VVVVVVVVV',
        cursor: 9,
        current: 'message',
        status: 'VALID',
        args: {
          command: { name: 'echo' },
          message: {
            value: 'a\nb',
            arg: ' a\\nb',
            status: 'VALID',
            message: ''
          }
        }
      }
    }
  ]);
};

exports.testTab = function(options) {
  helpers.audit(options, [
    {
      setup:    'echo a\\tb',
      check: {
        input:  'echo a\\tb',
        hints:            '',
        markup: 'VVVVVVVVV',
        cursor: 9,
        current: 'message',
        status: 'VALID',
        args: {
          command: { name: 'echo' },
          message: {
            value: 'a\tb',
            arg: ' a\\tb',
            status: 'VALID',
            message: ''
          }
        }
      }
    }
  ]);
};

exports.testEscape = function(options) {
  helpers.audit(options, [
    {
      
      
      setup:    'tsrsrsr a\\\\ b c',
      check: {
        input:  'tsrsrsr a\\\\ b c',
        hints:                 '',
        markup: 'VVVVVVVVVVVVVVV',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsrsrsr' },
          p1: { value: 'a\\', arg: ' a\\\\', status: 'VALID', message: '' },
          p2: { value: 'b', arg: ' b', status: 'VALID', message: '' },
          p3: { value: 'c', arg: ' c', status: 'VALID', message: '' },
        }
      }
    },
    {
      
      
      setup:    'tsrsrsr abc\\\\ndef asd asd',
      check: {
        input:  'tsrsrsr abc\\\\ndef asd asd',
        hints:                           '',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVVVV',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsrsrsr' },
          p1: {
            value: 'abc\\ndef',
            arg: ' abc\\\\ndef',
            status: 'VALID',
            message: ''
          },
          p2: { value: 'asd', arg: ' asd', status: 'VALID', message: '' },
          p3: { value: 'asd', arg: ' asd', status: 'VALID', message: '' },
        }
      }
    }
  ]);
};









