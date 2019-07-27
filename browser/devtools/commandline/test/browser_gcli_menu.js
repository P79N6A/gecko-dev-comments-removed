















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_menu.js");
}



exports.testOptions = function(options) {
  return helpers.audit(options, [
    {
      setup:    'tslong',
      check: {
        input:  'tslong',
        markup: 'VVVVVV',
        status: 'ERROR',
        hints: ' <msg> [options]',
        args: {
          msg: { value: undefined, status: 'INCOMPLETE' },
          num: { value: undefined, status: 'VALID' },
          sel: { value: undefined, status: 'VALID' },
          bool: { value: false, status: 'VALID' },
          bool2: { value: false, status: 'VALID' },
          sel2: { value: undefined, status: 'VALID' },
          num2: { value: undefined, status: 'VALID' }
        }
      }
    }
  ]);
};
