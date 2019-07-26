















'use strict';





var exports = {};

var TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testFileparser.js</p>";

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




var fileparser = require('gcli/util/fileparser');

var local = false;

exports.testGetPredictor = function(options) {
  if (!options.isNode || !local) {
    assert.log('Skipping tests due to install differences.');
    return;
  }

  var opts = { filetype: 'file', existing: 'yes' };
  var predictor = fileparser.getPredictor('/usr/locl/bin/nmp', opts);
  return predictor().then(function(replies) {
    assert.is(replies[0].name,
              '/usr/local/bin/npm',
              'predict npm');
  });
};
