






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testHistory.js</p>";

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




var History = require('gcli/history').History;

exports.setup = function() {
};

exports.shutdown = function() {
};

exports.testSimpleHistory = function () {
  var history = new History({});
  history.add('foo');
  history.add('bar');
  assert.is('bar', history.backward());
  assert.is('foo', history.backward());

  
  history.add('quux');
  assert.is('quux', history.backward());
  assert.is('bar', history.backward());
  assert.is('foo', history.backward());
};

exports.testBackwardsPastIndex = function () {
  var history = new History({});
  history.add('foo');
  history.add('bar');
  assert.is('bar', history.backward());
  assert.is('foo', history.backward());

  
  
  assert.is('foo', history.backward());
};

exports.testForwardsPastIndex = function () {
  var history = new History({});
  history.add('foo');
  history.add('bar');
  assert.is('bar', history.backward());
  assert.is('foo', history.backward());

  
  assert.is('bar', history.forward());

  
  assert.is('', history.forward());

  
  assert.is('', history.forward());
};


