















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_keyboard4.js");
}



exports.testIncrFloat = function(options) {
  return helpers.audit(options, [
    






    {
      setup: 'tsf -6.5<UP>',
      check: { input: 'tsf -6' }
    },
    {
      setup: 'tsf -6<UP>',
      check: { input: 'tsf -4.5' }
    },
    {
      setup: 'tsf -4.5<UP>',
      check: { input: 'tsf -3' }
    },
    {
      setup: 'tsf -4<UP>',
      check: { input: 'tsf -3' }
    },
    {
      setup: 'tsf -3<UP>',
      check: { input: 'tsf -1.5' }
    },
    {
      setup: 'tsf -1.5<UP>',
      check: { input: 'tsf 0' }
    },
    {
      setup: 'tsf 0<UP>',
      check: { input: 'tsf 1.5' }
    },
    {
      setup: 'tsf 1.5<UP>',
      check: { input: 'tsf 3' }
    },
    {
      setup: 'tsf 2<UP>',
      check: { input: 'tsf 3' }
    },
    {
      setup: 'tsf 3<UP>',
      check: { input: 'tsf 4.5' }
    },
    {
      setup: 'tsf 5<UP>',
      check: { input: 'tsf 6' }
    }
    






  ]);
};

exports.testDecrFloat = function(options) {
  return helpers.audit(options, [
    






    {
      setup: 'tsf -6.5<DOWN>',
      check: { input: 'tsf -6.5' }
    },
    {
      setup: 'tsf -6<DOWN>',
      check: { input: 'tsf -6.5' }
    },
    {
      setup: 'tsf -4.5<DOWN>',
      check: { input: 'tsf -6' }
    },
    {
      setup: 'tsf -4<DOWN>',
      check: { input: 'tsf -4.5' }
    },
    {
      setup: 'tsf -3<DOWN>',
      check: { input: 'tsf -4.5' }
    },
    {
      setup: 'tsf -1.5<DOWN>',
      check: { input: 'tsf -3' }
    },
    {
      setup: 'tsf 0<DOWN>',
      check: { input: 'tsf -1.5' }
    },
    {
      setup: 'tsf 1.5<DOWN>',
      check: { input: 'tsf 0' }
    },
    {
      setup: 'tsf 2<DOWN>',
      check: { input: 'tsf 1.5' }
    },
    {
      setup: 'tsf 3<DOWN>',
      check: { input: 'tsf 1.5' }
    },
    {
      setup: 'tsf 5<DOWN>',
      check: { input: 'tsf 4.5' }
    }
    






  ]);
};

exports.testIncrSelection = function(options) {
  



















};

exports.testDecrSelection = function(options) {
  








};
