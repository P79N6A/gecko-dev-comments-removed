















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_keyboard1.js");
}

var javascript = require('gcli/types/javascript');


exports.testSimple = function(options) {
  return helpers.audit(options, [
    {
      setup: 'tsela<TAB>',
      check: { input: 'tselarr ', cursor: 8 }
    },
    {
      setup: 'tsn di<TAB>',
      check: { input: 'tsn dif ', cursor: 8 }
    },
    {
      setup: 'tsg a<TAB>',
      check: { input: 'tsg aaa ', cursor: 8 }
    }
  ]);
};

exports.testScript = function(options) {
  return helpers.audit(options, [
    {
      skipRemainingIf: options.isRemote ||
              options.requisition.system.commands.get('{') == null,
      setup: '{ wind<TAB>',
      check: { input: '{ window' }
    },
    {
      setup: '{ window.docum<TAB>',
      check: { input: '{ window.document' }
    }
  ]);
};

exports.testJsdom = function(options) {
  return helpers.audit(options, [
    {
      skipIf: options.isRemote ||
              options.requisition.system.commands.get('{') == null,
      setup: '{ window.document.titl<TAB>',
      check: { input: '{ window.document.title ' }
    }
  ]);
};
