















'use strict';





var exports = {};

var TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testHistory.js</p>";

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




var History = require('gcli/ui/history').History;

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
