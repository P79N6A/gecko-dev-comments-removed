















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_remotexhr.js");
}









exports.testRemoteXhr = function(options) {
  return helpers.audit(options, [
    {
      skipRemainingIf: options.isRemote || options.isNode || options.isFirefox,
      setup:    'remote ',
      check: {
        input:  'remote ',
        hints:         '',
        markup: 'EEEEEEV',
        cursor: 7,
        current: '__command',
        status: 'ERROR',
        options: [ ],
        message: 'Can\'t use \'remote\'.',
        predictions: [ ],
        unassigned: [ ],
      }
    },
    {
      setup: 'connect remote',
      check: {
        args: {
          prefix: { value: 'remote' },
          url: { value: undefined }
        }
      },
      exec: {
        error: false
      }
    },
    {
      setup: 'disconnect remote',
      check: {
        args: {
          prefix: {
            value: function(front) {
              assert.is(front.prefix, 'remote', 'disconnecting remote');
            }
          }
        }
      },
      exec: {
        output: /^Removed [0-9]* commands.$/,
        type: 'string',
        error: false
      }
    },
    {
      setup: 'connect remote --method xhr',
      check: {
        args: {
          prefix: { value: 'remote' },
          url: { value: undefined }
        }
      },
      exec: {
        error: false
      }
    },
    {
      setup: 'disconnect remote',
      check: {
        args: {
          prefix: {
            value: function(front) {
              assert.is(front.prefix, 'remote', 'disconnecting remote');
            }
          }
        }
      },
      exec: {
        output: /^Removed [0-9]* commands.$/,
        type: 'string',
        error: false
      }
    },
    {
      setup: 'connect remote --method xhr',
      check: {
        args: {
          prefix: { value: 'remote' },
          url: { value: undefined }
        }
      },
      exec: {
        output: /^Added [0-9]* commands.$/,
        type: 'string',
        error: false
      }
    },
    {
      setup:    'remote ',
      check: {
        input:  'remote ',
        
        
        markup: 'IIIIIIV',
        status: 'ERROR',
        optionsIncludes: [
          'remote', 'remote cd', 'remote context', 'remote echo',
          'remote exec', 'remote exit', 'remote firefox', 'remote help',
          'remote intro', 'remote make'
        ],
        message: '',
        predictionsIncludes: [ 'remote' ],
        unassigned: [ ],
      }
    },
    {
      setup:    'remote echo hello world',
      check: {
        input:  'remote echo hello world',
        hints:                         '',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVV',
        cursor: 23,
        current: 'message',
        status: 'VALID',
        options: [ ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: 'remote echo' },
          message: {
            value: 'hello world',
            arg: ' hello world',
            status: 'VALID',
            message: ''
          }
        }
      },
      exec: {
        output: 'hello world',
        type: 'string',
        error: false
      }
    },
    {
      setup:    'remote exec ls',
      check: {
        input:  'remote exec ls',
        hints:                '',
        markup: 'VVVVVVVVVVVVVV',
        cursor: 14,
        current: 'command',
        status: 'VALID',
        options: [ ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: {
            value: 'ls',
            arg: ' ls',
            status: 'VALID',
            message: ''
          }
        }
      },
      exec: {
        
        type: 'output',
        error: false
      }
    },
    {
      setup:    'remote sleep mistake',
      check: {
        input:  'remote sleep mistake',
        hints:                      '',
        markup: 'VVVVVVVVVVVVVEEEEEEE',
        cursor: 20,
        current: 'length',
        status: 'ERROR',
        options: [ ],
        message: 'Can\'t convert "mistake" to a number.',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: 'remote sleep' },
          length: {
            value: undefined,
            arg: ' mistake',
            status: 'ERROR',
            message: 'Can\'t convert "mistake" to a number.'
          }
        }
      }
    },
    {
      setup:    'remote sleep 1',
      check: {
        input:  'remote sleep 1',
        hints:                 '',
        markup: 'VVVVVVVVVVVVVV',
        cursor: 14,
        current: 'length',
        status: 'VALID',
        options: [ ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: 'remote sleep' },
          length: { value: 1, arg: ' 1', status: 'VALID', message: '' }
        }
      },
      exec: {
        output: 'Done',
        type: 'string',
        error: false
      }
    },
    {
      setup:    'remote help ',
      skipIf: true, 
      check: {
        input:  'remote help ',
        hints:              '[search]',
        markup: 'VVVVVVVVVVVV',
        cursor: 12,
        current: 'search',
        status: 'VALID',
        options: [ ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: 'remote help' },
          search: {
            value: undefined,
            arg: '',
            status: 'VALID',
            message: ''
          }
        }
      },
      exec: {
        output: '',
        type: 'string',
        error: false
      }
    },
    {
      setup:    'remote intro',
      check: {
        input:  'remote intro',
        hints:              '',
        markup: 'VVVVVVVVVVVV',
        cursor: 12,
        current: '__command',
        status: 'VALID',
        options: [ ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: 'remote intro' }
        }
      },
      exec: {
        output: [
          /GCLI is an experiment/,
          /F1\/Escape/
        ],
        type: 'intro',
        error: false
      }
    },
    {
      setup:    'context remote',
      check: {
        input:  'context remote',
        
        markup: 'VVVVVVVVVVVVVV',
        cursor: 14,
        current: 'prefix',
        status: 'VALID',
        optionsContains: [
          'remote', 'remote cd', 'remote echo', 'remote exec', 'remote exit',
          'remote firefox', 'remote help', 'remote intro', 'remote make'
        ],
        message: '',
        
        
        
        
        
        unassigned: [ ],
        args: {
          command: { name: 'context' },
          prefix: {
            arg: ' remote',
            status: 'VALID',
            message: ''
          }
        }
      },
      exec: {
        output: 'Using remote as a command prefix',
        type: 'string',
        error: false
      }
    },
    {
      setup:    'exec ls',
      check: {
        input:  'exec ls',
        hints:         '',
        markup: 'VVVVVVV',
        cursor: 7,
        current: 'command',
        status: 'VALID',
        options: [ ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { value: 'ls', arg: ' ls', status: 'VALID', message: '' },
        }
      },
      exec: {
        
        type: 'output',
        error: false
      }
    },
    {
      setup:    'echo hello world',
      check: {
        input:  'echo hello world',
        hints:                  '',
        markup: 'VVVVVVVVVVVVVVVV',
        cursor: 16,
        current: 'message',
        status: 'VALID',
        options: [ ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: 'remote echo' },
          message: {
            value: 'hello world',
            arg: ' hello world',
            status: 'VALID',
            message: ''
          }
        }
      },
      exec: {
        output: /^hello world$/,
        type: 'string',
        error: false
      }
    },
    {
      setup:    'context',
      check: {
        input:  'context',
        hints:         ' [prefix]',
        markup: 'VVVVVVV',
        cursor: 7,
        current: '__command',
        status: 'VALID',
        optionsContains: [
          'remote', 'remote cd', 'remote echo', 'remote exec', 'remote exit',
          'remote firefox', 'remote help', 'remote intro', 'remote make'
        ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: 'context' },
          prefix: { value: undefined, arg: '', status: 'VALID', message: '' }
        }
      },
      exec: {
        output: 'Command prefix is unset',
        type: 'string',
        error: false
      }
    },
    {
      setup:    'disconnect ',
      check: {
        input:  'disconnect ',
        hints:             'remote',
        markup: 'VVVVVVVVVVV',
        cursor: 11,
        current: 'prefix',
        status: 'ERROR',
        options: [ 'remote' ],
        message: '',
        predictions: [ 'remote' ],
        unassigned: [ ],
        args: {
          command: { name: 'disconnect' },
          prefix: {
            value: undefined,
            arg: '',
            status: 'INCOMPLETE',
            message: 'Value required for \'prefix\'.'
          }
        }
      }
    },
    {
      setup:    'disconnect remote',
      check: {
        input:  'disconnect remote',
        hints:                   '',
        markup: 'VVVVVVVVVVVVVVVVV',
        status: 'VALID',
        message: '',
        unassigned: [ ],
        args: {
          prefix: {
            value: function(front) {
              assert.is(front.prefix, 'remote', 'disconnecting remote');
            },
            arg: ' remote',
            status: 'VALID',
            message: ''
          }
        }
      },
      exec: {
        output: /^Removed [0-9]* commands.$/,
        type: 'string',
        error: false
      }
    },
    {
      setup:    'remote ',
      check: {
        input:  'remote ',
        hints:         '',
        markup: 'EEEEEEV',
        cursor: 7,
        current: '__command',
        status: 'ERROR',
        options: [ ],
        message: 'Can\'t use \'remote\'.',
        predictions: [ ],
        unassigned: [ ],
      }
    }
  ]);
};
