






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testHistory.js</p>";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.runTests(options, exports);
  }).then(finish);
}



'use strict';


var History = require('gcli/history').History;

exports.testSimpleHistory = function (options) {
  var history = new History({});
  history.add('foo');
  history.add('bar');
  assert.is(history.backward(), 'bar');
  assert.is(history.backward(), 'foo');

  
  history.add('quux');
  assert.is(history.backward(), 'quux');
  assert.is(history.backward(), 'bar');
  assert.is(history.backward(), 'foo');
};

exports.testBackwardsPastIndex = function (options) {
  var history = new History({});
  history.add('foo');
  history.add('bar');
  assert.is(history.backward(), 'bar');
  assert.is(history.backward(), 'foo');

  
  
  assert.is(history.backward(), 'foo');
};

exports.testForwardsPastIndex = function (options) {
  var history = new History({});
  history.add('foo');
  history.add('bar');
  assert.is(history.backward(), 'bar');
  assert.is(history.backward(), 'foo');

  
  assert.is(history.forward(), 'bar');

  
  assert.is(history.forward(), '');

  
  assert.is(history.forward(), '');
};


