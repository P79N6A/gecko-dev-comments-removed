















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_js.js");
}




exports.setup = function(options) {
  if (jsTestDisallowed(options)) {
    return;
  }

  
  var win = options.requisition.environment.window;
  Object.defineProperty(win, 'donteval', {
    get: function() {
      assert.ok(false, 'donteval should not be used');
      console.trace();
      return { cant: '', touch: '', 'this': '' };
    },
    enumerable: true,
    configurable: true
  });
};

exports.shutdown = function(options) {
  if (jsTestDisallowed(options)) {
    return;
  }

  delete options.requisition.environment.window.donteval;
};

function jsTestDisallowed(options) {
  return options.isRemote || 
         options.isNode ||
         options.requisition.system.commands.get('{') == null;
}

exports.testBasic = function(options) {
  return helpers.audit(options, [
    {
      skipRemainingIf: jsTestDisallowed,
      setup:    '{',
      check: {
        input:  '{',
        hints:   '',
        markup: 'V',
        cursor: 1,
        current: 'javascript',
        status: 'ERROR',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: undefined,
            arg: '{',
            status: 'INCOMPLETE'
          }
        }
      }
    },
    {
      setup:    '{ ',
      check: {
        input:  '{ ',
        hints:    '',
        markup: 'VV',
        cursor: 2,
        current: 'javascript',
        status: 'ERROR',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: undefined,
            arg: '{ ',
            status: 'INCOMPLETE'
          }
        }
      }
    },
    {
      setup:    '{ w',
      check: {
        input:  '{ w',
        hints:     'indow',
        markup: 'VVI',
        cursor: 3,
        current: 'javascript',
        status: 'ERROR',
        predictionsContains: [ 'window' ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: 'w',
            arg: '{ w',
            status: 'INCOMPLETE'
          }
        }
      }
    },
    {
      setup:    '{ windo',
      check: {
        input:  '{ windo',
        hints:         'w',
        markup: 'VVIIIII',
        cursor: 7,
        current: 'javascript',
        status: 'ERROR',
        predictions: [ 'window' ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: 'windo',
            arg: '{ windo',
            status: 'INCOMPLETE'
          }
        }
      }
    },
    {
      setup:    '{ window',
      check: {
        input:  '{ window',
        hints:          '',
        markup: 'VVVVVVVV',
        cursor: 8,
        current: 'javascript',
        status: 'VALID',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: 'window',
            arg: '{ window',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      setup:    '{ window.do',
      check: {
        input:  '{ window.do',
        hints:             'cument',
        markup: 'VVIIIIIIIII',
        cursor: 11,
        current: 'javascript',
        status: 'ERROR',
        predictionsContains: [ 'window.document' ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: 'window.do',
            arg: '{ window.do',
            status: 'INCOMPLETE'
          }
        }
      }
    },
    {
      setup:    '{ window.document.title',
      check: {
        input:  '{ window.document.title',
        hints:                         '',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVV',
        cursor: 23,
        current: 'javascript',
        status: 'VALID',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: 'window.document.title',
            arg: '{ window.document.title',
            status: 'VALID',
            message: ''
          }
        }
      }
    }
  ]);
};

exports.testDocument = function(options) {
  return helpers.audit(options, [
    {
      skipRemainingIf: jsTestDisallowed,
      setup:    '{ docu',
      check: {
        input:  '{ docu',
        hints:        'ment',
        markup: 'VVIIII',
        cursor: 6,
        current: 'javascript',
        status: 'ERROR',
        predictions: [ 'document' ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: 'docu',
            arg: '{ docu',
            status: 'INCOMPLETE'
          }
        }
      }
    },
    {
      setup: '{ docu<TAB>',
      check: {
        input:  '{ document',
        hints:            '',
        markup: 'VVVVVVVVVV',
        cursor: 10,
        current: 'javascript',
        status: 'VALID',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: 'document',
            arg: '{ document',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      setup:    '{ document.titl',
      check: {
        input:  '{ document.titl',
        hints:                 'e',
        markup: 'VVIIIIIIIIIIIII',
        cursor: 15,
        current: 'javascript',
        status: 'ERROR',
        predictions: [ 'document.title' ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: 'document.titl',
            arg: '{ document.titl',
            status: 'INCOMPLETE'
          }
        }
      }
    },
    {
      setup: '{ document.titl<TAB>',
      check: {
        input:  '{ document.title ',
        hints:                   '',
        markup: 'VVVVVVVVVVVVVVVVV',
        cursor: 17,
        current: 'javascript',
        status: 'VALID',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: 'document.title',
            
            
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      setup:    '{ document.title',
      check: {
        input:  '{ document.title',
        hints:                  '',
        markup: 'VVVVVVVVVVVVVVVV',
        cursor: 16,
        current: 'javascript',
        status: 'VALID',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: 'document.title',
            arg: '{ document.title',
            status: 'VALID',
            message: ''
          }
        }
      }
    }
  ]);
};

exports.testDonteval = function(options) {
  return helpers.audit(options, [
    {
      skipRemainingIf: true, 
      setup:    '{ don',
      check: {
        input:  '{ don',
        hints:       'teval',
        markup: 'VVIII',
        cursor: 5,
        current: 'javascript',
        status: 'ERROR',
        predictions: [ 'donteval' ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: 'don',
            arg: '{ don',
            status: 'INCOMPLETE'
          }
        }
      }
    },
    {
      setup:    '{ donteval',
      check: {
        input:  '{ donteval',
        hints:            '',
        markup: 'VVVVVVVVVV',
        cursor: 10,
        current: 'javascript',
        status: 'VALID',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: 'donteval',
            arg: '{ donteval',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    



























    {
      setup:    '{ donteval.cant',
      check: {
        input:  '{ donteval.cant',
        hints:                 '',
        markup: 'VVVVVVVVVVVVVVV',
        cursor: 15,
        current: 'javascript',
        status: 'VALID',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: 'donteval.cant',
            arg: '{ donteval.cant',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      setup:    '{ donteval.xxx',
      check: {
        input:  '{ donteval.xxx',
        hints:                '',
        markup: 'VVVVVVVVVVVVVV',
        cursor: 14,
        current: 'javascript',
        status: 'VALID',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: '{' },
          javascript: {
            value: 'donteval.xxx',
            arg: '{ donteval.xxx',
            status: 'VALID',
            message: ''
          }
        }
      }
    }
  ]);
};

exports.testExec = function(options) {
  return helpers.audit(options, [
    {
      skipRemainingIf: jsTestDisallowed,
      setup:    '{ 1+1',
      check: {
        input:  '{ 1+1',
        hints:       '',
        markup: 'VVVVV',
        cursor: 5,
        current: 'javascript',
        status: 'VALID',
        options: [ ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          javascript: {
            value: '1+1',
            arg: '{ 1+1',
            status: 'VALID',
            message: ''
          }
        }
      },
      exec: {
        output: '2',
        type: 'number',
        error: false
      }
    },
    {
      setup:    '{ 1+1 }',
      check: {
        input:  '{ 1+1 }',
        hints:         '',
        markup: 'VVVVVVV',
        cursor: 7,
        current: 'javascript',
        status: 'VALID',
        options: [ ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          javascript: {
            value: '1+1',
            arg: '{ 1+1 }',
            status: 'VALID',
            message: ''
          }
        }
      },
      exec: {
        output: '2',
        type: 'number',
        error: false
      }
    },
    {
      setup:    '{ "hello"',
      check: {
        input:  '{ "hello"',
        hints:           '',
        markup: 'VVVVVVVVV',
        cursor: 9,
        current: 'javascript',
        status: 'VALID',
        options: [ ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          javascript: {
            value: '"hello"',
            arg: '{ "hello"',
            status: 'VALID',
            message: ''
          }
        }
      },
      exec: {
        output: 'hello',
        type: 'string',
        error: false
      }
    },
    {
      setup:    '{ "hello" + 1',
      check: {
        input:  '{ "hello" + 1',
        hints:               '',
        markup: 'VVVVVVVVVVVVV',
        cursor: 13,
        current: 'javascript',
        status: 'VALID',
        options: [ ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          javascript: {
            value: '"hello" + 1',
            arg: '{ "hello" + 1',
            status: 'VALID',
            message: ''
          }
        }
      },
      exec: {
        output: 'hello1',
        type: 'string',
        error: false
      }
    }
  ]);
};
