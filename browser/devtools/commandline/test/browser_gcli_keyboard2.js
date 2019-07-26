















'use strict';





var exports = {};

var TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testKeyboard2.js</p>";

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





exports.testIncr = function(options) {
  return helpers.audit(options, [
    
















    {
      setup: 'tsu -5<UP>',
      check: { input: 'tsu -3' }
    },
    {
      setup: 'tsu -4<UP>',
      check: { input: 'tsu -3' }
    },
    {
      setup: 'tsu -3<UP>',
      check: { input: 'tsu 0' }
    },
    {
      setup: 'tsu -2<UP>',
      check: { input: 'tsu 0' }
    },
    {
      setup: 'tsu -1<UP>',
      check: { input: 'tsu 0' }
    },
    {
      setup: 'tsu 0<UP>',
      check: { input: 'tsu 3' }
    },
    {
      setup: 'tsu 1<UP>',
      check: { input: 'tsu 3' }
    },
    {
      setup: 'tsu 2<UP>',
      check: { input: 'tsu 3' }
    },
    {
      setup: 'tsu 3<UP>',
      check: { input: 'tsu 6' }
    },
    {
      setup: 'tsu 4<UP>',
      check: { input: 'tsu 6' }
    },
    {
      setup: 'tsu 5<UP>',
      check: { input: 'tsu 6' }
    },
    {
      setup: 'tsu 6<UP>',
      check: { input: 'tsu 9' }
    },
    {
      setup: 'tsu 7<UP>',
      check: { input: 'tsu 9' }
    },
    {
      setup: 'tsu 8<UP>',
      check: { input: 'tsu 9' }
    },
    {
      setup: 'tsu 9<UP>',
      check: { input: 'tsu 10' }
    },
    {
      setup: 'tsu 10<UP>',
      check: { input: 'tsu 10' }
    }
    






  ]);
};
