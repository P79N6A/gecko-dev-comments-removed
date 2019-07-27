















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_context.js");
}



exports.testBaseline = function(options) {
  return helpers.audit(options, [
    
    
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
        hints:     ' deep down nested cmd',
        markup: 'III',
        cursor: 3,
        current: '__command',
        status: 'ERROR',
        predictionsContains: [ 'tsn deep down nested cmd', 'tsn ext', 'tsn exte' ],
        args: {
          command: { name: 'tsn' },
        }
      }
    }
  ]);
};

exports.testContext = function(options) {
  return helpers.audit(options, [
    
    {
      setup:    'context tsn',
      check: {
        input:  'context tsn',
        hints:             ' deep down nested cmd',
        markup: 'VVVVVVVVVVV',
        message: '',
        predictionsContains: [ 'tsn deep down nested cmd', 'tsn ext', 'tsn exte' ],
        args: {
          command: { name: 'context' },
          prefix: {
            value: options.requisition.system.commands.get('tsn'),
            status: 'VALID',
            message: ''
          }
        }
      },
      exec: {
        output: 'Using tsn as a command prefix'
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
            status: 'INCOMPLETE'
          }
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
          }
        }
      },
      exec: {
        output: 'Exec: tsnExt text=test'
      }
    },
    {
      setup:    'tsn',
      check: {
        input:  'tsn',
        hints:     ' deep down nested cmd',
        markup: 'III',
        message: '',
        predictionsContains: [ 'tsn deep down nested cmd', 'tsn ext', 'tsn exte' ],
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
          toggle: { value: true, arg: ' true', status: 'VALID', message: '' }
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
        predictions: [ 'tsn ext', 'tsn exte', 'tsn exten', 'tsn extend' ],
        unassigned: [ ],
        args: {
          command: { name: 'context' },
          prefix: {
            value: options.requisition.system.commands.get('tsn ext'),
            status: 'VALID',
            message: ''
          }
        }
      },
      exec: {
        output: 'Can\'t use \'tsn ext\' as a prefix because it is not a parent command.',
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
        type: 'string',
        error: false
      }
    }
  ]);
};
