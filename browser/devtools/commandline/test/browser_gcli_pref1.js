















'use strict';





var exports = {};

var TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testPref1.js</p>";

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





exports.testPrefShowStatus = function(options) {
  return helpers.audit(options, [
    {
      skipRemainingIf: options.requisition.system.commands.get('pref') == null,
      setup:    'pref s',
      check: {
        typed:  'pref s',
        hints:        'et',
        markup: 'IIIIVI',
        status: 'ERROR'
      }
    },
    {
      setup:    'pref show',
      check: {
        typed:  'pref show',
        hints:           ' <setting>',
        markup: 'VVVVVVVVV',
        status: 'ERROR'
      }
    },
    {
      setup:    'pref show ',
      check: {
        typed:  'pref show ',
        hints:            'allowSet',
        markup: 'VVVVVVVVVV',
        status: 'ERROR'
      }
    },
    {
      setup:    'pref show tempTBo',
      check: {
        typed:  'pref show tempTBo',
        hints:                   'ol',
        markup: 'VVVVVVVVVVIIIIIII',
        status: 'ERROR'
      }
    },
    {
      setup:    'pref show tempTBool',
      check: {
        typed:  'pref show tempTBool',
        markup: 'VVVVVVVVVVVVVVVVVVV',
        status: 'VALID',
        hints:  ''
      }
    },
    {
      setup:    'pref show tempTBool 4',
      check: {
        typed:  'pref show tempTBool 4',
        markup: 'VVVVVVVVVVVVVVVVVVVVE',
        status: 'ERROR',
        hints:  ''
      }
    },
    {
      setup:    'pref show tempNumber 4',
      check: {
        typed:  'pref show tempNumber 4',
        markup: 'VVVVVVVVVVVVVVVVVVVVVE',
        status: 'ERROR',
        hints:  ''
      }
    }
  ]);
};

exports.testPrefSetStatus = function(options) {
  return helpers.audit(options, [
    {
      skipRemainingIf: options.requisition.system.commands.get('pref') == null,
      setup:    'pref s',
      check: {
        typed:  'pref s',
        hints:        'et',
        markup: 'IIIIVI',
        status: 'ERROR'
      }
    },
    {
      setup:    'pref set',
      check: {
        typed:  'pref set',
        hints:          ' <setting> <value>',
        markup: 'VVVVVVVV',
        status: 'ERROR'
      }
    },
    {
      setup:    'pref xxx',
      check: {
        typed:  'pref xxx',
        markup: 'IIIIVIII',
        status: 'ERROR'
      }
    },
    {
      setup:    'pref set ',
      check: {
        typed:  'pref set ',
        hints:           'allowSet <value>',
        markup: 'VVVVVVVVV',
        status: 'ERROR'
      }
    },
    {
      setup:    'pref set tempTBo',
      check: {
        typed:  'pref set tempTBo',
        hints:                  'ol <value>',
        markup: 'VVVVVVVVVIIIIIII',
        status: 'ERROR'
      }
    },
    {
      setup:    'pref set tempTBool 4',
      check: {
        typed:  'pref set tempTBool 4',
        markup: 'VVVVVVVVVVVVVVVVVVVE',
        status: 'ERROR',
        hints: ''
      }
    },
    {
      setup:    'pref set tempNumber 4',
      check: {
        typed:  'pref set tempNumber 4',
        markup: 'VVVVVVVVVVVVVVVVVVVVV',
        status: 'VALID',
        hints: ''
      }
    }
  ]);
};
