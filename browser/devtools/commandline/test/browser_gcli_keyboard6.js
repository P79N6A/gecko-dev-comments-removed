















'use strict';





var exports = {};

var TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testKeyboard6.js</p>";

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





exports.testCompleteUp = function(options) {
  return helpers.audit(options, [
    {
      setup: 'tsn e<UP><TAB>',
      check: { input: 'tsn extend ' }
    },
    {
      setup: 'tsn e<UP><UP><TAB>',
      check: { input: 'tsn exten ' }
    },
    {
      setup: 'tsn e<UP><UP><UP><TAB>',
      check: { input: 'tsn exte ' }
    },
    {
      setup: 'tsn e<UP><UP><UP><UP><TAB>',
      check: { input: 'tsn ext ' }
    },
    {
      setup: 'tsn e<UP><UP><UP><UP><UP><TAB>',
      check: { input: 'tsn extend ' }
    },
    {
      setup: 'tsn e<UP><UP><UP><UP><UP><UP><TAB>',
      check: { input: 'tsn exten ' }
    },
    {
      setup: 'tsn e<UP><UP><UP><UP><UP><UP><UP><TAB>',
      check: { input: 'tsn exte ' }
    },
    {
      setup: 'tsn e<UP><UP><UP><UP><UP><UP><UP><UP><TAB>',
      check: { input: 'tsn ext ' }
    }
  ]);
};
