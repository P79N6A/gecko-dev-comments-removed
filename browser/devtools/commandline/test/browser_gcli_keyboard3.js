






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testKeyboard3.js</p>";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.runTests(options, exports);
  }).then(finish);
}



'use strict';

var javascript = require('gcli/types/javascript');


var canon = require('gcli/canon');

var tempWindow;

exports.setup = function(options) {
  mockCommands.setup();

  tempWindow = javascript.getGlobalObject();
  javascript.setGlobalObject(options.window);
};

exports.shutdown = function(options) {
  javascript.setGlobalObject(tempWindow);
  tempWindow = undefined;

  mockCommands.shutdown();
};

exports.testScript = function(options) {
  return helpers.audit(options, [
    {
      skipIf: function commandJsMissing() {
        return canon.getCommand('{') == null;
      },
      setup: '{ wind<TAB>',
      check: { input: '{ window' }
    },
    {
      skipIf: function commandJsMissing() {
        return canon.getCommand('{') == null;
      },
      setup: '{ window.docum<TAB>',
      check: { input: '{ window.document' }
    }
  ]);
};

exports.testJsdom = function(options) {
  return helpers.audit(options, [
    {
      skipIf: function jsDomOrCommandJsMissing() {
        return options.isJsdom || canon.getCommand('{') == null;
      },
      setup: '{ window.document.titl<TAB>',
      check: { input: '{ window.document.title ' }
    }
  ]);
};



