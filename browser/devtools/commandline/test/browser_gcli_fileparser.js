






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testFileparser.js</p>";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.runTests(options, exports);
  }).then(finish);
}



'use strict';


var fileparser = require('util/fileparser');

var local = false;

exports.testGetPredictor = function(options) {
  if (!options.isNode || !local) {
    return;
  }

  var options = { filetype: 'file', existing: 'yes' };
  var predictor = fileparser.getPredictor('/usr/locl/bin/nmp', options);
  return predictor().then(function(replies) {
    assert.is(replies[0].name,
              '/usr/local/bin/npm',
              'predict npm');
  });
};


