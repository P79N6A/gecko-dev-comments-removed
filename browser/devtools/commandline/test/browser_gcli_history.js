















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_history.js");
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
