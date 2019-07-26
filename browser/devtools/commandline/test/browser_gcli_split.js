






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testSplit.js</p>";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.runTests(options, exports);
  }).then(finish);
}



'use strict';


var Requisition = require('gcli/cli').Requisition;
var canon = require('gcli/canon');


exports.setup = function(options) {
  mockCommands.setup();
};

exports.shutdown = function(options) {
  mockCommands.shutdown();
};


exports.testSplitSimple = function(options) {
  var args;
  var requ = new Requisition();

  args = requ._tokenize('s');
  requ._split(args);
  assert.is(0, args.length);
  assert.is('s', requ.commandAssignment.arg.text);
};

exports.testFlatCommand = function(options) {
  var args;
  var requ = new Requisition();

  args = requ._tokenize('tsv');
  requ._split(args);
  assert.is(0, args.length);
  assert.is('tsv', requ.commandAssignment.value.name);

  args = requ._tokenize('tsv a b');
  requ._split(args);
  assert.is('tsv', requ.commandAssignment.value.name);
  assert.is(2, args.length);
  assert.is('a', args[0].text);
  assert.is('b', args[1].text);
};

exports.testJavascript = function(options) {
  if (!canon.getCommand('{')) {
    assert.log('Skipping testJavascript because { is not registered');
    return;
  }

  var args;
  var requ = new Requisition();

  args = requ._tokenize('{');
  requ._split(args);
  assert.is(1, args.length);
  assert.is('', args[0].text);
  assert.is('', requ.commandAssignment.arg.text);
  assert.is('{', requ.commandAssignment.value.name);
};




