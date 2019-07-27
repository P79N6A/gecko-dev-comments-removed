















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_split.js");
}



var cli = require('gcli/cli');

exports.testSplitSimple = function(options) {
  var args = cli.tokenize('s');
  options.requisition._split(args);
  assert.is(args.length, 0);
  assert.is(options.requisition.commandAssignment.arg.text, 's');
};

exports.testFlatCommand = function(options) {
  var args = cli.tokenize('tsv');
  options.requisition._split(args);
  assert.is(args.length, 0);
  assert.is(options.requisition.commandAssignment.value.name, 'tsv');

  args = cli.tokenize('tsv a b');
  options.requisition._split(args);
  assert.is(options.requisition.commandAssignment.value.name, 'tsv');
  assert.is(args.length, 2);
  assert.is(args[0].text, 'a');
  assert.is(args[1].text, 'b');
};

exports.testJavascript = function(options) {
  if (!options.requisition.system.commands.get('{')) {
    assert.log('Skipping testJavascript because { is not registered');
    return;
  }

  var args = cli.tokenize('{');
  options.requisition._split(args);
  assert.is(args.length, 1);
  assert.is(args[0].text, '');
  assert.is(options.requisition.commandAssignment.arg.text, '');
  assert.is(options.requisition.commandAssignment.value.name, '{');
};


