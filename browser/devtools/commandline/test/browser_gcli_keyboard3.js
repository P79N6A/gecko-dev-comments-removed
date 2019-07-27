















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_keyboard3.js");
}



exports.testDecr = function(options) {
  return helpers.audit(options, [
    














    {
      setup: 'tsu -5<DOWN>',
      check: { input: 'tsu -5' }
    },
    {
      setup: 'tsu -4<DOWN>',
      check: { input: 'tsu -5' }
    },
    {
      setup: 'tsu -3<DOWN>',
      check: { input: 'tsu -5' }
    },
    {
      setup: 'tsu -2<DOWN>',
      check: { input: 'tsu -3' }
    },
    {
      setup: 'tsu -1<DOWN>',
      check: { input: 'tsu -3' }
    },
    {
      setup: 'tsu 0<DOWN>',
      check: { input: 'tsu -3' }
    },
    {
      setup: 'tsu 1<DOWN>',
      check: { input: 'tsu 0' }
    },
    {
      setup: 'tsu 2<DOWN>',
      check: { input: 'tsu 0' }
    },
    {
      setup: 'tsu 3<DOWN>',
      check: { input: 'tsu 0' }
    },
    {
      setup: 'tsu 4<DOWN>',
      check: { input: 'tsu 3' }
    },
    {
      setup: 'tsu 5<DOWN>',
      check: { input: 'tsu 3' }
    },
    {
      setup: 'tsu 6<DOWN>',
      check: { input: 'tsu 3' }
    },
    {
      setup: 'tsu 7<DOWN>',
      check: { input: 'tsu 6' }
    },
    {
      setup: 'tsu 8<DOWN>',
      check: { input: 'tsu 6' }
    },
    {
      setup: 'tsu 9<DOWN>',
      check: { input: 'tsu 6' }
    },
    {
      setup: 'tsu 10<DOWN>',
      check: { input: 'tsu 9' }
    }
    






  ]);
};
