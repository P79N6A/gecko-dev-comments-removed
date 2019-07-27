















'use strict';





var exports = {};

var TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testPref2.js</p>";

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

  var allowSet = settings.getSetting('allowSet');
  var initialAllowSet = allowSet.value;
  allowSet.value = false;

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
      skipRemainingIf: options.isNoDom,
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
        output: [ /void your warranty/, /I promise/ ]
      },
      post: function() {
        assert.is(mockSettings.tempNumber.value, 42, 'still set to 42');
        allowSet.value = true;
      }
    },
    {
      setup:    'pref set tempNumber 4',
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

        allowSet.value = initialAllowSet;
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
