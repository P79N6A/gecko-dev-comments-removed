















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_focus.js");
}



exports.testBasic = function(options) {
  return helpers.audit(options, [
    {
      name: 'exec setup',
      setup: function() {
        
        helpers.focusInput(options);
        return helpers.setInput(options, 'echo hi');
      },
      check: { },
      exec: { }
    },
    {
      setup:    'tsn deep',
      check: {
        input:  'tsn deep',
        hints:          ' down nested cmd',
        markup: 'IIIVIIII',
        cursor: 8,
        status: 'ERROR',
        outputState: 'false:default',
        tooltipState: 'false:default'
      }
    },
    {
      setup:    'tsn deep<TAB>',
      check: {
        input:  'tsn deep down nested cmd ',
        hints:                           '',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVVVV',
        cursor: 25,
        status: 'VALID',
        outputState: 'false:default',
        tooltipState: 'false:default'
      }
    }
  ]);
};
