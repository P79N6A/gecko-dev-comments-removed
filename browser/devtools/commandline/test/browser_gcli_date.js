






















var exports = {};

const TEST_URI = "data:text/html;charset=utf-8,<p id='gcli-input'>gcli-testDate.js</p>";

function test() {
  helpers.addTabWithToolbar(TEST_URI, function(options) {
    return helpers.runTests(options, exports);
  }).then(finish);
}



'use strict';



var types = require('gcli/types');
var Argument = require('gcli/argument').Argument;
var Status = require('gcli/types').Status;




exports.setup = function(options) {
  mockCommands.setup();
};

exports.shutdown = function(options) {
  mockCommands.shutdown();
};


exports.testParse = function(options) {
  var date = types.createType('date');
  return date.parse(new Argument('now')).then(function(conversion) {
    
    
    
    
    var gap = new Date().getTime() - conversion.value.getTime();
    assert.ok(gap < 60000, 'now is less than a minute away');

    assert.is(conversion.getStatus(), Status.VALID, 'now parse');
  });
};

exports.testMaxMin = function(options) {
  var max = new Date();
  var min = new Date();
  var date = types.createType({ name: 'date', max: max, min: min });
  assert.is(date.getMax(), max, 'max setup');

  var incremented = date.increment(min);
  assert.is(incremented, max, 'incremented');
};

exports.testIncrement = function(options) {
  var date = types.createType('date');
  return date.parse(new Argument('now')).then(function(conversion) {
    var plusOne = date.increment(conversion.value);
    var minusOne = date.decrement(plusOne);

    
    var gap = new Date().getTime() - minusOne.getTime();
    assert.ok(gap < 60000, 'now is less than a minute away');
  });
};

exports.testInput = function(options) {
  return helpers.audit(options, [
    {
      
      skipRemainingIf: options.isFirefox,
      setup:    'tsdate 2001-01-01 1980-01-03',
      check: {
        input:  'tsdate 2001-01-01 1980-01-03',
        hints:                              '',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVVVVVVV',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsdate' },
          d1: {
            value: function(d1) {
              assert.is(d1.getFullYear(), 2001, 'd1 year');
              assert.is(d1.getMonth(), 0, 'd1 month');
              assert.is(d1.getDate(), 1, 'd1 date');
              assert.is(d1.getHours(), 0, 'd1 hours');
              assert.is(d1.getMinutes(), 0, 'd1 minutes');
              assert.is(d1.getSeconds(), 0, 'd1 seconds');
              assert.is(d1.getMilliseconds(), 0, 'd1 millis');
            },
            arg: ' 2001-01-01',
            status: 'VALID',
            message: ''
          },
          d2: {
            value: function(d2) {
              assert.is(d2.getFullYear(), 1980, 'd1 year');
              assert.is(d2.getMonth(), 0, 'd1 month');
              assert.is(d2.getDate(), 3, 'd1 date');
              assert.is(d2.getHours(), 0, 'd1 hours');
              assert.is(d2.getMinutes(), 0, 'd1 minutes');
              assert.is(d2.getSeconds(), 0, 'd1 seconds');
              assert.is(d2.getMilliseconds(), 0, 'd1 millis');
            },
            arg: ' 1980-01-03',
            status: 'VALID',
            message: ''
          },
        }
      },
      exec: {
        output: [ /^Exec: tsdate/, /2001/, /1980/ ],
        completed: true,
        type: 'string',
        error: false
      }
    }
  ]);
};

exports.testIncrDecr = function(options) {
  return helpers.audit(options, [
    {
      
      skipRemainingIf: options.isFirefox,
      setup:    'tsdate 2001-01-01<UP>',
      check: {
        input:  'tsdate 2001-01-02',
        hints:                    ' <d2>',
        markup: 'VVVVVVVVVVVVVVVVV',
        status: 'ERROR',
        message: '',
        args: {
          command: { name: 'tsdate' },
          d1: {
            value: function(d1) {
              assert.is(d1.getFullYear(), 2001, 'd1 year');
              assert.is(d1.getMonth(), 0, 'd1 month');
              assert.is(d1.getDate(), 2, 'd1 date');
              assert.is(d1.getHours(), 0, 'd1 hours');
              assert.is(d1.getMinutes(), 0, 'd1 minutes');
              assert.is(d1.getSeconds(), 0, 'd1 seconds');
              assert.is(d1.getMilliseconds(), 0, 'd1 millis');
            },
            arg: ' 2001-01-02',
            status: 'VALID',
            message: ''
          },
          d2: {
            value: undefined,
            status: 'INCOMPLETE',
            message: ''
          },
        }
      }
    },
    {
      
      setup:    'tsdate 2001-02-01<DOWN>',
      check: {
        input:  'tsdate 2001-01-31',
        hints:                    ' <d2>',
        markup: 'VVVVVVVVVVVVVVVVV',
        status: 'ERROR',
        message: '',
        args: {
          command: { name: 'tsdate' },
          d1: {
            value: function(d1) {
              assert.is(d1.getFullYear(), 2001, 'd1 year');
              assert.is(d1.getMonth(), 0, 'd1 month');
              assert.is(d1.getDate(), 31, 'd1 date');
              assert.is(d1.getHours(), 0, 'd1 hours');
              assert.is(d1.getMinutes(), 0, 'd1 minutes');
              assert.is(d1.getSeconds(), 0, 'd1 seconds');
              assert.is(d1.getMilliseconds(), 0, 'd1 millis');
            },
            arg: ' 2001-01-31',
            status: 'VALID',
            message: ''
          },
          d2: {
            value: undefined,
            status: 'INCOMPLETE',
            message: ''
          },
        }
      }
    },
    {
      
      setup:    'tsdate 2001-02-01 "27 feb 2000"<UP>',
      check: {
        input:  'tsdate 2001-02-01 "2000-02-28"',
        hints:                                '',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVVVVVVVVV',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsdate' },
          d1: {
            value: function(d1) {
              assert.is(d1.getFullYear(), 2001, 'd1 year');
              assert.is(d1.getMonth(), 1, 'd1 month');
              assert.is(d1.getDate(), 1, 'd1 date');
              assert.is(d1.getHours(), 0, 'd1 hours');
              assert.is(d1.getMinutes(), 0, 'd1 minutes');
              assert.is(d1.getSeconds(), 0, 'd1 seconds');
              assert.is(d1.getMilliseconds(), 0, 'd1 millis');
            },
            arg: ' 2001-02-01',
            status: 'VALID',
            message: ''
          },
          d2: {
            value: function(d2) {
              assert.is(d2.getFullYear(), 2000, 'd2 year');
              assert.is(d2.getMonth(), 1, 'd2 month');
              assert.is(d2.getDate(), 28, 'd2 date');
              assert.is(d2.getHours(), 0, 'd2 hours');
              assert.is(d2.getMinutes(), 0, 'd2 minutes');
              assert.is(d2.getSeconds(), 0, 'd2 seconds');
              assert.is(d2.getMilliseconds(), 0, 'd2 millis');
            },
            arg: ' "2000-02-28"',
            status: 'VALID',
            message: ''
          },
        }
      }
    }
  ]);
};



