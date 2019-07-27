















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_date.js");
}




var Status = require('gcli/types/types').Status;

exports.testParse = function(options) {
  var date = options.requisition.system.types.createType('date');
  return date.parseString('now').then(function(conversion) {
    
    
    
    
    var gap = new Date().getTime() - conversion.value.getTime();
    assert.ok(gap < 60000, 'now is less than a minute away');

    assert.is(conversion.getStatus(), Status.VALID, 'now parse');
  });
};

exports.testMaxMin = function(options) {
  var max = new Date();
  var min = new Date();
  var types = options.requisition.system.types;
  var date = types.createType({ name: 'date', max: max, min: min });
  assert.is(date.getMax(), max, 'max setup');

  var incremented = date.nudge(min, 1);
  assert.is(incremented, max, 'incremented');
};

exports.testIncrement = function(options) {
  var date = options.requisition.system.types.createType('date');
  return date.parseString('now').then(function(conversion) {
    var plusOne = date.nudge(conversion.value, 1);
    var minusOne = date.nudge(plusOne, -1);

    
    var gap = new Date().getTime() - minusOne.getTime();
    assert.ok(gap < 60000, 'now is less than a minute away');
  });
};

exports.testInput = function(options) {
  return helpers.audit(options, [
    {
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
              assert.is(d2.getFullYear(), 1980, 'd2 year');
              assert.is(d2.getMonth(), 0, 'd2 month');
              assert.is(d2.getDate(), 3, 'd2 date');
              assert.is(d2.getHours(), 0, 'd2 hours');
              assert.is(d2.getMinutes(), 0, 'd2 minutes');
              assert.is(d2.getSeconds(), 0, 'd2 seconds');
              assert.is(d2.getMilliseconds(), 0, 'd2 millis');
            },
            arg: ' 1980-01-03',
            status: 'VALID',
            message: ''
          },
        }
      },
      exec: {
        output: [ /^Exec: tsdate/, /2001/, /1980/ ],
        type: 'testCommandOutput',
        error: false
      }
    },
    {
      setup:    'tsdate 2001/01/01 1980/01/03',
      check: {
        input:  'tsdate 2001/01/01 1980/01/03',
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
            arg: ' 2001/01/01',
            status: 'VALID',
            message: ''
          },
          d2: {
            value: function(d2) {
              assert.is(d2.getFullYear(), 1980, 'd2 year');
              assert.is(d2.getMonth(), 0, 'd2 month');
              assert.is(d2.getDate(), 3, 'd2 date');
              assert.is(d2.getHours(), 0, 'd2 hours');
              assert.is(d2.getMinutes(), 0, 'd2 minutes');
              assert.is(d2.getSeconds(), 0, 'd2 seconds');
              assert.is(d2.getMilliseconds(), 0, 'd2 millis');
            },
            arg: ' 1980/01/03',
            status: 'VALID',
            message: ''
          },
        }
      },
      exec: {
        output: [ /^Exec: tsdate/, /2001/, /1980/ ],
        type: 'testCommandOutput',
        error: false
      }
    },
    {
      setup:    'tsdate now today',
      check: {
        input:  'tsdate now today',
        hints:                  '',
        markup: 'VVVVVVVVVVVVVVVV',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsdate' },
          d1: {
            value: function(d1) {
              
              
              
              
              assert.ok(d1.getTime() - new Date().getTime() < 30 * 1000,
                        'd1 time');
            },
            arg: ' now',
            status: 'VALID',
            message: ''
          },
          d2: {
            value: function(d2) {
              
              assert.ok(d2.getTime() - new Date().getTime() < 30 * 1000,
                        'd2 time');
            },
            arg: ' today',
            status: 'VALID',
            message: ''
          },
        }
      },
      exec: {
        output: [ /^Exec: tsdate/, new Date().getFullYear() ],
        type: 'testCommandOutput',
        error: false
      }
    },
    {
      setup:    'tsdate yesterday tomorrow',
      check: {
        input:  'tsdate yesterday tomorrow',
        hints:                           '',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVVVV',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsdate' },
          d1: {
            value: function(d1) {
              var compare = new Date().getTime() - (24 * 60 * 60 * 1000);
              
              assert.ok(d1.getTime() - compare < 30 * 1000,
                        'd1 time');
            },
            arg: ' yesterday',
            status: 'VALID',
            message: ''
          },
          d2: {
            value: function(d2) {
              var compare = new Date().getTime() + (24 * 60 * 60 * 1000);
              
              assert.ok(d2.getTime() - compare < 30 * 1000,
                        'd2 time');
            },
            arg: ' tomorrow',
            status: 'VALID',
            message: ''
          },
        }
      },
      exec: {
        output: [ /^Exec: tsdate/, new Date().getFullYear() ],
        type: 'testCommandOutput',
        error: false
      }
    }
  ]);
};

exports.testIncrDecr = function(options) {
  return helpers.audit(options, [
    {
      
      skipRemainingIf: options.isNode,
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
            status: 'INCOMPLETE'
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
            status: 'INCOMPLETE'
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
