















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_intro.js");
}



exports.testIntroStatus = function(options) {
  return helpers.audit(options, [
    {
      skipRemainingIf: function commandIntroMissing() {
        return options.requisition.system.commands.get('intro') == null;
      },
      setup:    'intro',
      check: {
        typed:  'intro',
        markup: 'VVVVV',
        status: 'VALID',
        hints: ''
      }
    },
    {
      setup:    'intro foo',
      check: {
        typed:  'intro foo',
        markup: 'VVVVVVEEE',
        status: 'ERROR',
        hints: ''
      }
    },
    {
      setup:    'intro',
      check: {
        typed:  'intro',
        markup: 'VVVVV',
        status: 'VALID',
        hints: ''
      },
      exec: {
        output: [
          /command\s*line/,
          /help/,
          /F1/,
          /Escape/
        ]
      }
    }
  ]);
};
