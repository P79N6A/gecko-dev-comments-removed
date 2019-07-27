















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_completion1.js");
}



exports.testActivate = function(options) {
  return helpers.audit(options, [
    {
      setup: '',
      check: {
        hints: ''
      }
    },
    {
      setup: ' ',
      check: {
        hints: ''
      }
    },
    {
      setup: 'tsr',
      check: {
        hints: ' <text>'
      }
    },
    {
      setup: 'tsr ',
      check: {
        hints: '<text>'
      }
    },
    {
      setup: 'tsr b',
      check: {
        hints: ''
      }
    },
    {
      setup: 'tsb',
      check: {
        hints: ' [toggle]'
      }
    },
    {
      setup: 'tsm',
      check: {
        hints: ' <abc> <txt> <num>'
      }
    },
    {
      setup: 'tsm ',
      check: {
        hints: 'a <txt> <num>'
      }
    },
    {
      setup: 'tsm a',
      check: {
        hints: ' <txt> <num>'
      }
    },
    {
      setup: 'tsm a ',
      check: {
        hints: '<txt> <num>'
      }
    },
    {
      setup: 'tsm a  ',
      check: {
        hints: '<txt> <num>'
      }
    },
    {
      setup: 'tsm a  d',
      check: {
        hints: ' <num>'
      }
    },
    {
      setup: 'tsm a "d d"',
      check: {
        hints: ' <num>'
      }
    },
    {
      setup: 'tsm a "d ',
      check: {
        hints: ' <num>'
      }
    },
    {
      setup: 'tsm a "d d" ',
      check: {
        hints: '<num>'
      }
    },
    {
      setup: 'tsm a "d d ',
      check: {
        hints: ' <num>'
      }
    },
    {
      setup: 'tsm d r',
      check: {
        hints: ' <num>'
      }
    },
    {
      setup: 'tsm a d ',
      check: {
        hints: '<num>'
      }
    },
    {
      setup: 'tsm a d 4',
      check: {
        hints: ''
      }
    },
    {
      setup: 'tsg',
      check: {
        hints: ' <solo> [options]'
      }
    },
    {
      setup: 'tsg ',
      check: {
        hints: 'aaa [options]'
      }
    },
    {
      setup: 'tsg a',
      check: {
        hints: 'aa [options]'
      }
    },
    {
      setup: 'tsg b',
      check: {
        hints: 'bb [options]'
      }
    },
    {
      skipIf: options.isPhantomjs, 
      setup: 'tsg d',
      check: {
        hints: ' [options] -> ccc'
      }
    },
    {
      setup: 'tsg aa',
      check: {
        hints: 'a [options]'
      }
    },
    {
      setup: 'tsg aaa',
      check: {
        hints: ' [options]'
      }
    },
    {
      setup: 'tsg aaa ',
      check: {
        hints: '[options]'
      }
    },
    {
      setup: 'tsg aaa d',
      check: {
        hints: ' [options]'
      }
    },
    {
      setup: 'tsg aaa dddddd',
      check: {
        hints: ' [options]'
      }
    },
    {
      setup: 'tsg aaa dddddd ',
      check: {
        hints: '[options]'
      }
    },
    {
      setup: 'tsg aaa "d',
      check: {
        hints: ' [options]'
      }
    },
    {
      setup: 'tsg aaa "d d',
      check: {
        hints: ' [options]'
      }
    },
    {
      setup: 'tsg aaa "d d"',
      check: {
        hints: ' [options]'
      }
    },
    {
      setup: 'tsn ex ',
      check: {
        hints: ''
      }
    },
    {
      setup: 'selarr',
      check: {
        hints: ' -> tselarr'
      }
    },
    {
      setup: 'tselar 1',
      check: {
        hints: ''
      }
    },
    {
      name: 'tselar |1',
      setup: function() {
        return helpers.setInput(options, 'tselar 1', 7);
      },
      check: {
        hints: ''
      }
    },
    {
      name: 'tselar| 1',
      setup: function() {
        return helpers.setInput(options, 'tselar 1', 6);
      },
      check: {
        hints: ' -> tselarr'
      }
    },
    {
      name: 'tsela|r 1',
      setup: function() {
        return helpers.setInput(options, 'tselar 1', 5);
      },
      check: {
        hints: ' -> tselarr'
      }
    },
  ]);
};
