















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_union.js");
}




exports.testDefault = function(options) {
  return helpers.audit(options, [
    {
      setup:    'unionc1',
      check: {
        input:  'unionc1',
        markup: 'VVVVVVV',
        hints:         ' <first>',
        status: 'ERROR',
        args: {
          first: {
            value: undefined,
            arg: '',
            status: 'INCOMPLETE'
          }
        }
      }
    },
    {
      setup:    'unionc1 three',
      check: {
        input:  'unionc1 three',
        markup: 'VVVVVVVVVVVVV',
        hints:               '',
        status: 'VALID',
        args: {
          first: {
            value: function(data) {
              assert.is(Object.keys(data).length, 2, 'union3 Object.keys');
              assert.is(data.type, 'string', 'union3 val type');
              assert.is(data.string, 'three', 'union3 val string');
            },
            arg: ' three',
            status: 'VALID'
          }
        }
      },
      exec: {
        output: [
          /"type": ?"string"/,
          /"string": ?"three"/
        ]
      },
      post: function(output, text) {
        var data = output.data.first;
        assert.is(Object.keys(data).length, 2, 'union3 Object.keys');
        assert.is(data.type, 'string', 'union3 val type');
        assert.is(data.string, 'three', 'union3 val string');
      }
    },
    {
      setup:    'unionc1 one',
      check: {
        input:  'unionc1 one',
        markup: 'VVVVVVVVVVV',
        hints:             '',
        status: 'VALID',
        args: {
          first: {
            value: function(data) {
              assert.is(Object.keys(data).length, 2, 'union1 Object.keys');
              assert.is(data.type, 'selection', 'union1 val type');
              assert.is(data.selection, 1, 'union1 val selection');
            },
            arg: ' one',
            status: 'VALID'
          }
        }
      },
      exec: {
        output: [
          /"type": ?"selection"/,
          /"selection": ?1/
        ]
      },
      post: function(output, text) {
        var data = output.data.first;
        assert.is(Object.keys(data).length, 2, 'union1 Object.keys');
        assert.is(data.type, 'selection', 'union1 val type');
        assert.is(data.selection, 1, 'union1 val selection');
      }
    },
    {
      skipIf: options.isPhantomjs, 
      setup:    'unionc1 5',
      check: {
        input:  'unionc1 5',
        markup: 'VVVVVVVVV',
        hints:           ' -> two',
        predictions: [ 'two' ],
        status: 'VALID',
        args: {
          first: {
            value: function(data) {
              assert.is(Object.keys(data).length, 2, 'union5 Object.keys');
              assert.is(data.type, 'number', 'union5 val type');
              assert.is(data.number, 5, 'union5 val number');
            },
            arg: ' 5',
            status: 'VALID'
          }
        }
      },
      exec: {
        output: [
          /"type": ?"number"/,
          /"number": ?5/
        ]
      },
      post: function(output, text) {
        var data = output.data.first;
        assert.is(Object.keys(data).length, 2, 'union5 Object.keys');
        assert.is(data.type, 'number', 'union5 val type');
        assert.is(data.number, 5, 'union5 val number');
      }
    },
    {
      skipIf: options.isPhantomjs, 
      setup:    'unionc2 on',
      check: {
        input:  'unionc2 on',
        hints:            'e',
        markup: 'VVVVVVVVII',
        current: 'first',
        status: 'ERROR',
        predictionsContains: [
          'one',
          'http://on/',
          'https://on/'
        ],
        args: {
          command: { name: 'unionc2' },
          first: {
            value: undefined,
            arg: ' on',
            status: 'INCOMPLETE',
            message: 'Can\'t use \'on\'.'
          },
        }
      }
    }
  ]);
};
