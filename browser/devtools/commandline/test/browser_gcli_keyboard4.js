















'use strict';





var exports = {};

var TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testKeyboard4.js</p>";

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





exports.testIncrFloat = function(options) {
  return helpers.audit(options, [
    






    {
      setup: 'tsf -6.5<UP>',
      check: { input: 'tsf -6' }
    },
    {
      setup: 'tsf -6<UP>',
      check: { input: 'tsf -4.5' }
    },
    {
      setup: 'tsf -4.5<UP>',
      check: { input: 'tsf -3' }
    },
    {
      setup: 'tsf -4<UP>',
      check: { input: 'tsf -3' }
    },
    {
      setup: 'tsf -3<UP>',
      check: { input: 'tsf -1.5' }
    },
    {
      setup: 'tsf -1.5<UP>',
      check: { input: 'tsf 0' }
    },
    {
      setup: 'tsf 0<UP>',
      check: { input: 'tsf 1.5' }
    },
    {
      setup: 'tsf 1.5<UP>',
      check: { input: 'tsf 3' }
    },
    {
      setup: 'tsf 2<UP>',
      check: { input: 'tsf 3' }
    },
    {
      setup: 'tsf 3<UP>',
      check: { input: 'tsf 4.5' }
    },
    {
      setup: 'tsf 5<UP>',
      check: { input: 'tsf 6' }
    }
    






  ]);
};

exports.testDecrFloat = function(options) {
  return helpers.audit(options, [
    






    {
      setup: 'tsf -6.5<DOWN>',
      check: { input: 'tsf -6.5' }
    },
    {
      setup: 'tsf -6<DOWN>',
      check: { input: 'tsf -6.5' }
    },
    {
      setup: 'tsf -4.5<DOWN>',
      check: { input: 'tsf -6' }
    },
    {
      setup: 'tsf -4<DOWN>',
      check: { input: 'tsf -4.5' }
    },
    {
      setup: 'tsf -3<DOWN>',
      check: { input: 'tsf -4.5' }
    },
    {
      setup: 'tsf -1.5<DOWN>',
      check: { input: 'tsf -3' }
    },
    {
      setup: 'tsf 0<DOWN>',
      check: { input: 'tsf -1.5' }
    },
    {
      setup: 'tsf 1.5<DOWN>',
      check: { input: 'tsf 0' }
    },
    {
      setup: 'tsf 2<DOWN>',
      check: { input: 'tsf 1.5' }
    },
    {
      setup: 'tsf 3<DOWN>',
      check: { input: 'tsf 1.5' }
    },
    {
      setup: 'tsf 5<DOWN>',
      check: { input: 'tsf 4.5' }
    }
    






  ]);
};

exports.testIncrSelection = function(options) {
  



















};

exports.testDecrSelection = function(options) {
  








};
