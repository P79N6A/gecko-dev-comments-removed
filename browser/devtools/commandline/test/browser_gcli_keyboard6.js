















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_keyboard6.js");
}



exports.testCompleteUp = function(options) {
  return helpers.audit(options, [
    {
      setup: 'tsn e<UP><TAB>',
      check: { input: 'tsn extend ' }
    },
    {
      setup: 'tsn e<UP><UP><TAB>',
      check: { input: 'tsn exten ' }
    },
    {
      setup: 'tsn e<UP><UP><UP><TAB>',
      check: { input: 'tsn exte ' }
    },
    {
      setup: 'tsn e<UP><UP><UP><UP><TAB>',
      check: { input: 'tsn ext ' }
    },
    {
      setup: 'tsn e<UP><UP><UP><UP><UP><TAB>',
      check: { input: 'tsn extend ' }
    },
    {
      setup: 'tsn e<UP><UP><UP><UP><UP><UP><TAB>',
      check: { input: 'tsn exten ' }
    },
    {
      setup: 'tsn e<UP><UP><UP><UP><UP><UP><UP><TAB>',
      check: { input: 'tsn exte ' }
    },
    {
      setup: 'tsn e<UP><UP><UP><UP><UP><UP><UP><UP><TAB>',
      check: { input: 'tsn ext ' }
    }
  ]);
};
