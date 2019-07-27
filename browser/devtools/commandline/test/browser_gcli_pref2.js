















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_pref2.js");
}



var mockSettings = require('./mockSettings');

exports.testPrefExec = function(options) {
  if (options.requisition.system.commands.get('pref') == null) {
    assert.log('Skipping test; missing pref command.');
    return;
  }

  if (options.isRemote) {
    assert.log('Skipping test which assumes local settings.');
    return;
  }

  assert.is(mockSettings.tempNumber.value, 42, 'set to 42');

  return helpers.audit(options, [
    {
      
      
      skipIf: options.isRemote,
      setup: 'pref set tempNumber 4',
      check: {
        setting: { value: mockSettings.tempNumber },
        args: { value: { value: 4 } }
      }
    },
    {
      setup:    'pref set tempNumber 4',
      check: {
        input:  'pref set tempNumber 4',
        hints:                       '',
        markup: 'VVVVVVVVVVVVVVVVVVVVV',
        cursor: 21,
        current: 'value',
        status: 'VALID',
        predictions: [ ],
        unassigned: [ ],
        args: {
          command: { name: 'pref set' },
          setting: {
            arg: ' tempNumber',
            status: 'VALID',
            message: ''
          },
          value: {
            arg: ' 4',
            status: 'VALID',
            message: ''
          }
        }
      },
      exec: {
        output: ''
      },
      post: function() {
        assert.is(mockSettings.tempNumber.value, 4, 'set to 4');
      }
    },
    {
      setup:    'pref reset tempNumber',
      check: {
        args: {
          command: { name: 'pref reset' },
          setting: { value: mockSettings.tempNumber }
        }
      },
      exec: {
        output: ''
      },
      post: function() {
        assert.is(mockSettings.tempNumber.value, 42, 'reset to 42');
      }
    },
    {
      skipRemainingIf: function commandPrefListMissing() {
        return options.requisition.system.commands.get('pref list') == null;
      },
      setup:    'pref list tempNum',
      check: {
        args: {
          command: { name: 'pref list' },
          search: { value: 'tempNum' }
        }
      },
      exec: {
        output: /tempNum/
      }
    },
  ]);
};
