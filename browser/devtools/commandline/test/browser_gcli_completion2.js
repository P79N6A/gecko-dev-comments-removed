















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_completion2.js");
}



exports.testLong = function(options) {
  return helpers.audit(options, [
    {
      setup:    'tslong --sel',
      check: {
        input:  'tslong --sel',
        hints:              ' <selection> <msg> [options]',
        markup: 'VVVVVVVIIIII'
      }
    },
    {
      setup:    'tslong --sel<TAB>',
      check: {
        input:  'tslong --sel ',
        hints:               'space <msg> [options]',
        markup: 'VVVVVVVIIIIIV'
      }
    },
    {
      setup:    'tslong --sel ',
      check: {
        input:  'tslong --sel ',
        hints:               'space <msg> [options]',
        markup: 'VVVVVVVIIIIIV'
      }
    },
    {
      setup:    'tslong --sel s',
      check: {
        input:  'tslong --sel s',
        hints:                'pace <msg> [options]',
        markup: 'VVVVVVVIIIIIVI'
      }
    },
    {
      setup:    'tslong --num ',
      check: {
        input:  'tslong --num ',
        hints:               '<number> <msg> [options]',
        markup: 'VVVVVVVIIIIIV'
      }
    },
    {
      setup:    'tslong --num 42',
      check: {
        input:  'tslong --num 42',
        hints:                 ' <msg> [options]',
        markup: 'VVVVVVVVVVVVVVV'
      }
    },
    {
      setup:    'tslong --num 42 ',
      check: {
        input:  'tslong --num 42 ',
        hints:                  '<msg> [options]',
        markup: 'VVVVVVVVVVVVVVVV'
      }
    },
    {
      setup:    'tslong --num 42 --se',
      check: {
        input:  'tslong --num 42 --se',
        hints:                      'l <msg> [options]',
        markup: 'VVVVVVVVVVVVVVVVIIII'
      }
    },
    {
      setup:    'tslong --num 42 --se<TAB>',
      check: {
        input:  'tslong --num 42 --sel ',
        hints:                        'space <msg> [options]',
        markup: 'VVVVVVVVVVVVVVVVIIIIIV'
      }
    },
    {
      setup:    'tslong --num 42 --se<TAB><TAB>',
      check: {
        input:  'tslong --num 42 --sel space ',
        hints:                              '<msg> [options]',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVVVVVVV'
      }
    },
    {
      setup:    'tslong --num 42 --sel ',
      check: {
        input:  'tslong --num 42 --sel ',
        hints:                        'space <msg> [options]',
        markup: 'VVVVVVVVVVVVVVVVIIIIIV'
      }
    },
    {
      setup:    'tslong --num 42 --sel space ',
      check: {
        input:  'tslong --num 42 --sel space ',
        hints:                              '<msg> [options]',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVVVVVVV'
      }
    }
  ]);
};

exports.testNoTab = function(options) {
  return helpers.audit(options, [
    {
      setup:    'tss<TAB>',
      check: {
        input:  'tss ',
        markup: 'VVVV',
        hints: ''
      }
    },
    {
      setup:    'tss<TAB><TAB>',
      check: {
        input:  'tss ',
        markup: 'VVVV',
        hints: ''
      }
    },
    {
      setup:    'xxxx',
      check: {
        input:  'xxxx',
        markup: 'EEEE',
        hints: ''
      }
    },
    {
      name: '<TAB>',
      setup: function() {
        
        return helpers.pressTab(options);
      },
      check: {
        input:  'xxxx',
        markup: 'EEEE',
        hints: ''
      }
    }
  ]);
};

exports.testOutstanding = function(options) {
  
  











};

exports.testCompleteIntoOptional = function(options) {
  
  return helpers.audit(options, [
    {
      setup:    'tso ',
      check: {
        typed:  'tso ',
        hints:      '[text]',
        markup: 'VVVV',
        status: 'VALID'
      }
    },
    {
      setup:    'tso<TAB>',
      check: {
        typed:  'tso ',
        hints:      '[text]',
        markup: 'VVVV',
        status: 'VALID'
      }
    }
  ]);
};

exports.testSpaceComplete = function(options) {
  return helpers.audit(options, [
    {
      setup:    'tslong --sel2 wit',
      check: {
        input:  'tslong --sel2 wit',
        hints:                   'h space <msg> [options]',
        markup: 'VVVVVVVIIIIIIVIII',
        cursor: 17,
        current: 'sel2',
        status: 'ERROR',
        tooltipState: 'true:importantFieldFlag',
        args: {
          command: { name: 'tslong' },
          msg: { status: 'INCOMPLETE' },
          num: { status: 'VALID' },
          sel: { status: 'VALID' },
          bool: { value: false, status: 'VALID' },
          num2: { status: 'VALID' },
          bool2: { value: false, status: 'VALID' },
          sel2: { arg: ' --sel2 wit', status: 'INCOMPLETE' }
        }
      }
    },
    {
      setup:    'tslong --sel2 wit<TAB>',
      check: {
        input:  'tslong --sel2 \'with space\' ',
        hints:                             '<msg> [options]',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVVVVVV',
        cursor: 27,
        current: 'sel2',
        status: 'ERROR',
        tooltipState: 'true:importantFieldFlag',
        args: {
          command: { name: 'tslong' },
          msg: { status: 'INCOMPLETE' },
          num: { status: 'VALID' },
          sel: { status: 'VALID' },
          bool: { value: false, status: 'VALID' },
          num2: { status: 'VALID' },
          bool2: { value: false, status: 'VALID' },
          sel2: {
            value: 'with space',
            arg: ' --sel2 \'with space\' ',
            status: 'VALID'
          }
        }
      }
    }
  ]);
};
