















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_filesystem.js");
}



var filesystem = require('gcli/util/filesystem');

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
