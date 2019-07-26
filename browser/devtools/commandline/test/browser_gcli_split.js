






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testSplit.js</p>";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.runTests(options, exports);
  }).then(finish);
}



'use strict';


var cli = require('gcli/cli');
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
  var requisition = new Requisition();

  args = cli.tokenize('s');
  requisition._split(args);
  assert.is(args.length, 0);
  assert.is(requisition.commandAssignment.arg.text, 's');
};

exports.testFlatCommand = function(options) {
  var args;
  var requisition = new Requisition();

  args = cli.tokenize('tsv');
  requisition._split(args);
  assert.is(args.length, 0);
  assert.is(requisition.commandAssignment.value.name, 'tsv');

  args = cli.tokenize('tsv a b');
  requisition._split(args);
  assert.is(requisition.commandAssignment.value.name, 'tsv');
  assert.is(args.length, 2);
  assert.is(args[0].text, 'a');
  assert.is(args[1].text, 'b');
};

exports.testJavascript = function(options) {
  if (!canon.getCommand('{')) {
    assert.log('Skipping testJavascript because { is not registered');
    return;
  }

  var args;
  var requisition = new Requisition();

  args = cli.tokenize('{');
  requisition._split(args);
  assert.is(args.length, 1);
  assert.is(args[0].text, '');
  assert.is(requisition.commandAssignment.arg.text, '');
  assert.is(requisition.commandAssignment.value.name, '{');
};




