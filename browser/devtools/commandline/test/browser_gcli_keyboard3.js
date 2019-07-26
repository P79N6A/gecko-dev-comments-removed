















'use strict';





var exports = {};

var TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testKeyboard3.js</p>";

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





exports.testDecr = function(options) {
  return helpers.audit(options, [
    














    {
      setup: 'tsu -5<DOWN>',
      check: { input: 'tsu -5' }
    },
    {
      setup: 'tsu -4<DOWN>',
      check: { input: 'tsu -5' }
    },
    {
      setup: 'tsu -3<DOWN>',
      check: { input: 'tsu -5' }
    },
    {
      setup: 'tsu -2<DOWN>',
      check: { input: 'tsu -3' }
    },
    {
      setup: 'tsu -1<DOWN>',
      check: { input: 'tsu -3' }
    },
    {
      setup: 'tsu 0<DOWN>',
      check: { input: 'tsu -3' }
    },
    {
      setup: 'tsu 1<DOWN>',
      check: { input: 'tsu 0' }
    },
    {
      setup: 'tsu 2<DOWN>',
      check: { input: 'tsu 0' }
    },
    {
      setup: 'tsu 3<DOWN>',
      check: { input: 'tsu 0' }
    },
    {
      setup: 'tsu 4<DOWN>',
      check: { input: 'tsu 3' }
    },
    {
      setup: 'tsu 5<DOWN>',
      check: { input: 'tsu 3' }
    },
    {
      setup: 'tsu 6<DOWN>',
      check: { input: 'tsu 3' }
    },
    {
      setup: 'tsu 7<DOWN>',
      check: { input: 'tsu 6' }
    },
    {
      setup: 'tsu 8<DOWN>',
      check: { input: 'tsu 6' }
    },
    {
      setup: 'tsu 9<DOWN>',
      check: { input: 'tsu 6' }
    },
    {
      setup: 'tsu 10<DOWN>',
      check: { input: 'tsu 9' }
    }
    






  ]);
};
