






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testContext.js</p>";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.runTests(options, exports);
  }).then(finish);
}



'use strict';



var cli = require('gcli/cli');

var origLogErrors = undefined;

exports.setup = function(options) {
  mockCommands.setup();

  origLogErrors = cli.logErrors;
  cli.logErrors = false;
};

exports.shutdown = function(options) {
  mockCommands.shutdown();

  cli.logErrors = origLogErrors;
  origLogErrors = undefined;
};

exports.testBaseline = function(options) {
  helpers.audit(options, [
    
    
    {
      setup:    'ext',
      check: {
        input:  'ext',
        hints:     ' -> context',
        markup: 'III',
        message: '',
        predictions: [ 'context', 'tsn ext', 'tsn exte', 'tsn exten', 'tsn extend' ],
        unassigned: [ ],
      }
    },
    {
      setup:    'ext test',
      check: {
        input:  'ext test',
        hints:          '',
        markup: 'IIIVEEEE',
        status: 'ERROR',
        message: 'Too many arguments',
        unassigned: [ ' test' ],
      }
    },
    {
      setup:    'tsn',
      check: {
        input:  'tsn',
        hints:     '',
        markup: 'III',
        cursor: 3,
        current: '__command',
        status: 'ERROR',
        predictionsContains: [ 'tsn', 'tsn deep', 'tsn ext', 'tsn exte' ],
        args: {
          command: { name: 'tsn' },
        }
      }
    }
  ]);
};

exports.testContext = function(options) {
  helpers.audit(options, [
    
    {
      setup:    'context tsn',
      check: {
        input:  'context tsn',
        hints:             '',
        markup: 'VVVVVVVVVVV',
        message: '',
        predictionsContains: [ 'tsn', 'tsn deep', 'tsn ext', 'tsn exte' ],
        args: {
          command: { name: 'context' },
          prefix: {
            value: mockCommands.commands.tsn,
            status: 'VALID',
            message: ''
          },
        }
      },
      exec: {
        output: 'Using tsn as a command prefix',
        completed: true,
      }
    },
    
    {
      setup:    'ext',
      check: {
        input:  'ext',
        hints:     ' <text>',
        markup: 'VVV',
        predictions: [ 'tsn ext', 'tsn exte', 'tsn exten', 'tsn extend' ],
        args: {
          command: { name: 'tsn ext' },
          text: {
            value: undefined,
            arg: '',
            status: 'INCOMPLETE',
            message: ''
          },
        }
      }
    },
    {
      setup:    'ext test',
      check: {
        input:  'ext test',
        hints:          '',
        markup: 'VVVVVVVV',
        args: {
          command: { name: 'tsn ext' },
          text: {
            value: 'test',
            arg: ' test',
            status: 'VALID',
            message: ''
          },
        }
      },
      exec: {
        output: 'Exec: tsnExt text=test',
        completed: true,
      }
    },
    {
      setup:    'tsn',
      check: {
        input:  'tsn',
        hints:     '',
        markup: 'III',
        message: '',
        predictionsContains: [ 'tsn', 'tsn deep', 'tsn ext', 'tsn exte' ],
        args: {
          command: { name: 'tsn' },
        }
      }
    },
    
    {
      setup:    'tsb true',
      check: {
        input:  'tsb true',
        hints:          '',
        markup: 'VVVVVVVV',
        options: [ 'true' ],
        message: '',
        predictions: [ 'true' ],
        unassigned: [ ],
        args: {
          command: { name: 'tsb' },
          toggle: { value: true, arg: ' true', status: 'VALID', message: '' },
        }
      }
    },
    {
      
      setup: 'context tsn ext',
      skip: true
    },
    {
      setup:    'context "tsn ext"',
      check: {
        input:  'context "tsn ext"',
        hints:                   '',
        markup: 'VVVVVVVVVVVVVVVVV',
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: 'context' },
          prefix: {
            value: mockCommands.commands.tsnExt,
            status: 'VALID',
            message: ''
          }
        }
      },
      exec: {
        output: 'Error: Can\'t use \'tsn ext\' as a prefix because it is not a parent command.',
        completed: true,
        error: true
      }
    },
    

























    {
      setup:    'context',
      check: {
        input:  'context',
        hints:         ' [prefix]',
        markup: 'VVVVVVV',
        status: 'VALID',
        unassigned: [ ],
        args: {
          command: { name: 'context' },
          prefix: { value: undefined, arg: '', status: 'VALID', message: '' },
        }
      },
      exec: {
        output: 'Command prefix is unset',
        completed: true,
        type: 'string',
        error: false
      }
    }
  ]);
};



