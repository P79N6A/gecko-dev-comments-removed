






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testShort.js</p>";

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
      setup:    'tshidden -v',
      check: {
        input:  'tshidden -v',
        hints:             ' <string>',
        markup: 'VVVVVVVVVII',
        cursor: 11,
        current: 'visible',
        status: 'ERROR',
        options: [ ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: 'tshidden' },
          visible: {
            value: undefined,
            arg: ' -v',
            status: 'INCOMPLETE',
            message: ''
          },
          invisiblestring: {
            value: undefined,
            arg: '',
            status: 'VALID',
            message: ''
          },
          invisibleboolean: {
            value: false,
            arg: '',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      setup:    'tshidden -v v',
      check: {
        input:  'tshidden -v v',
        hints:               '',
        markup: 'VVVVVVVVVVVVV',
        cursor: 13,
        current: 'visible',
        status: 'VALID',
        options: [ ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: 'tshidden' },
          visible: {
            value: 'v',
            arg: ' -v v',
            status: 'VALID',
            message: ''
          },
          invisiblestring: {
            value: undefined,
            arg: '',
            status: 'VALID',
            message: ''
          },
          invisibleboolean: {
            value: false,
            arg: '',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      setup:    'tshidden -i i',
      check: {
        input:  'tshidden -i i',
        hints:               ' [options]',
        markup: 'VVVVVVVVVVVVV',
        cursor: 13,
        current: 'invisiblestring',
        status: 'VALID',
        options: [ ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: 'tshidden' },
          visible: {
            value: undefined,
            arg: '',
            status: 'VALID',
            message: ''
          },
          invisiblestring: {
            value: 'i',
            arg: ' -i i',
            status: 'VALID',
            message: ''
          },
          invisibleboolean: {
            value: false,
            arg: '',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      setup:    'tshidden -b',
      check: {
        input:  'tshidden -b',
        hints:             ' [options]',
        markup: 'VVVVVVVVVVV',
        cursor: 11,
        current: 'invisibleboolean',
        status: 'VALID',
        options: [ ],
        message: '',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: 'tshidden' },
          visible: {
            value: undefined,
            arg: '',
            status: 'VALID',
            message: ''
          },
          invisiblestring: {
            value: undefined,
            arg: '',
            status: 'VALID',
            message: ''
          },
          invisibleboolean: {
            value: true,
            arg: ' -b',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      setup:    'tshidden -j',
      check: {
        input:  'tshidden -j',
        hints:             ' [options]',
        markup: 'VVVVVVVVVEE',
        cursor: 11,
        current: '__unassigned',
        status: 'ERROR',
        options: [ ],
        message: 'Can\'t use \'-j\'.',
        predictions: [ ],
        unassigned: [ ' -j' ],
        args: {
          command: { name: 'tshidden' },
          visible: {
            value: undefined,
            arg: '',
            status: 'VALID',
            message: ''
          },
          invisiblestring: {
            value: undefined,
            arg: '',
            status: 'VALID',
            message: ''
          },
          invisibleboolean: {
            value: false,
            arg: '',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      setup:    'tshidden -v jj -b --',
      check: {
        input:  'tshidden -v jj -b --',
        hints:                      '',
        markup: 'VVVVVVVVVVVVVVVVVVEE',
        cursor: 20,
        current: '__unassigned',
        status: 'ERROR',
        options: [ ],
        message: 'Can\'t use \'--\'.',
        predictions: [ ],
        unassigned: [ ' --' ],
        args: {
          command: { name: 'tshidden' },
          visible: {
            value: 'jj',
            arg: ' -v jj',
            status: 'VALID',
            message: ''
          },
          invisiblestring: {
            value: undefined,
            arg: '',
            status: 'VALID',
            message: ''
          },
          invisibleboolean: {
            value: true,
            arg: ' -b',
            status: 'VALID',
            message: ''
          }
        }
      }
    }
  ]);
};



