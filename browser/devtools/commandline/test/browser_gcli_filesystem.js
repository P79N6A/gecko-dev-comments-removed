






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testFilesystem.js</p>";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.runTests(options, exports);
  }).then(finish);
}



'use strict';



var filesystem = require('util/filesystem');

exports.testSplit = function(options) {
  if (!options.isNode) {
    return;
  }

  helpers.arrayIs(filesystem.split('', '/'),
                  [ '.' ],
                  'split <blank>');

  helpers.arrayIs(filesystem.split('a', '/'),
                  [ 'a' ],
                  'split a');

  helpers.arrayIs(filesystem.split('a/b/c', '/'),
                  [ 'a', 'b', 'c' ],
                  'split a/b/c');

  helpers.arrayIs(filesystem.split('/a/b/c/', '/'),
                  [ 'a', 'b', 'c' ],
                  'split a/b/c');

  helpers.arrayIs(filesystem.split('/a/b///c/', '/'),
                  [ 'a', 'b', 'c' ],
                  'split a/b/c');
};

exports.testJoin = function(options) {
  if (!options.isNode) {
    return;
  }

  assert.is(filesystem.join('usr', 'local', 'bin'),
            'usr/local/bin',
            'join to usr/local/bin');
};


