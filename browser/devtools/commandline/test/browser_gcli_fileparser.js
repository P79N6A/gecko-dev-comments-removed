















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_fileparser.js");
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
