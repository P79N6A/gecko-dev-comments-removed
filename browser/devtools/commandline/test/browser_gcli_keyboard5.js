















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_keyboard5.js");
}



exports.testCompleteDown = function(options) {
  return helpers.audit(options, [
    {
      setup: 'tsn e<DOWN><DOWN><DOWN><DOWN><DOWN><TAB>',
      check: { input: 'tsn exte ' }
    },
    {
      setup: 'tsn e<DOWN><DOWN><DOWN><DOWN><TAB>',
      check: { input: 'tsn ext ' }
    },
    {
      setup: 'tsn e<DOWN><DOWN><DOWN><TAB>',
      check: { input: 'tsn extend ' }
    },
    {
      setup: 'tsn e<DOWN><DOWN><TAB>',
      check: { input: 'tsn exten ' }
    },
    {
      setup: 'tsn e<DOWN><TAB>',
      check: { input: 'tsn exte ' }
    },
    {
      setup: 'tsn e<TAB>',
      check: { input: 'tsn ext ' }
    }
  ]);
};
