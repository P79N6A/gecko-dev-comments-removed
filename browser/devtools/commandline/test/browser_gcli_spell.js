














'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_spell.js");
}


var spell = require('gcli/util/spell');

exports.testSpellerSimple = function(options) {
  var alternatives = [
    'window', 'document', 'InstallTrigger', 'requirejs', 'require','define',
    'console', 'location', 'constructor', 'getInterface', 'external', 'sidebar'
  ];

  assert.is(spell.correct('document', alternatives), 'document');
  assert.is(spell.correct('documen', alternatives), 'document');
  assert.is(spell.correct('ocument', alternatives), 'document');
  assert.is(spell.correct('odcument', alternatives), 'document');

  assert.is(spell.correct('=========', alternatives), undefined);
};

exports.testRank = function(options) {
  var distances = spell.rank('fred', [ 'banana', 'fred', 'ed', 'red', 'FRED' ]);

  assert.is(distances.length, 5, 'rank length');

  assert.is(distances[0].name, 'fred', 'fred name #0');
  assert.is(distances[1].name, 'FRED', 'FRED name #1');
  assert.is(distances[2].name, 'red', 'red name #2');
  assert.is(distances[3].name, 'ed', 'ed name #3');
  assert.is(distances[4].name, 'banana', 'banana name #4');

  assert.is(distances[0].dist, 0, 'fred dist 0');
  assert.is(distances[1].dist, 4, 'FRED dist 4');
  assert.is(distances[2].dist, 10, 'red dist 10');
  assert.is(distances[3].dist, 20, 'ed dist 20');
  assert.is(distances[4].dist, 100, 'banana dist 100');
};

exports.testRank2 = function(options) {
  var distances = spell.rank('caps', [ 'CAPS', 'false' ]);
  assert.is(JSON.stringify(distances),
            '[{"name":"CAPS","dist":4},{"name":"false","dist":50}]',
            'spell.rank("caps", [ "CAPS", "false" ]');
};

exports.testDistancePrefix = function(options) {
  assert.is(spell.distancePrefix('fred', 'freddy'), 0, 'distancePrefix fred');
  assert.is(spell.distancePrefix('FRED', 'freddy'), 4, 'distancePrefix FRED');
};
