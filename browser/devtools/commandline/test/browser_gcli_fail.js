















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_fail.js");
}



exports.testBasic = function(options) {
  return helpers.audit(options, [
    {
      setup: 'tsfail reject',
      exec: {
        output: 'rejected promise',
        type: 'error',
        error: true
      }
    },
    {
      setup: 'tsfail rejecttyped',
      exec: {
        output: '54',
        type: 'number',
        error: true
      }
    },
    {
      setup: 'tsfail throwerror',
      exec: {
        output: /thrown error$/,
        type: 'error',
        error: true
      }
    },
    {
      setup: 'tsfail throwstring',
      exec: {
        output: 'thrown string',
        type: 'error',
        error: true
      }
    },
    {
      setup: 'tsfail noerror',
      exec: {
        output: 'no error',
        type: 'string',
        error: false
      }
    }
  ]);
};
