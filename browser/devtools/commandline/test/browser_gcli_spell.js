





















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testSpell.js</p>";

function test() {
  var tests = Object.keys(exports);
  
  tests.sort(function(t1, t2) {
    if (t1 == "setup" || t2 == "shutdown") return -1;
    if (t2 == "setup" || t1 == "shutdown") return 1;
    return 0;
  });
  info("Running tests: " + tests.join(", "))
  tests = tests.map(function(test) { return exports[test]; });
  DeveloperToolbarTest.test(TEST_URI, tests, true);
}




var spell = require('gcli/types/spell');

exports.setup = function() {
};

exports.shutdown = function() {
};

exports.testSpellerSimple = function(options) {
  var alternatives = Object.keys(options.window);

  assert.is(spell.correct('document', alternatives), 'document');
  assert.is(spell.correct('documen', alternatives), 'document');

  if (options.isJsdom) {
    assert.log('jsdom is weird, skipping some tests');
  }
  else {
    assert.is(spell.correct('ocument', alternatives), 'document');
  }
  assert.is(spell.correct('odcument', alternatives), 'document');

  assert.is(spell.correct('=========', alternatives), undefined);
};



