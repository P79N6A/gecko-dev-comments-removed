



var EXPORTED_SYMBOLS = ['Assert', 'Expect'];

const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

var broker = {}; Cu.import('resource://mozmill/driver/msgbroker.js', broker);
var errors = {}; Cu.import('resource://mozmill/modules/errors.js', errors);
var stack = {}; Cu.import('resource://mozmill/modules/stack.js', stack);












var Assert = function () {}

Assert.prototype = {

  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  _deepEqual: function (actual, expected) {
    
    if (actual === expected) {
      return true;

    
    
    } else if (actual instanceof Date && expected instanceof Date) {
      return actual.getTime() === expected.getTime();

    
    
    } else if (typeof actual != 'object' && typeof expected != 'object') {
      return actual == expected;

    
    
    
    
    
    
    } else {
      return this._objEquiv(actual, expected);
    }
  },

  _objEquiv: function (a, b) {
    if (a == null || a == undefined || b == null || b == undefined)
      return false;
    
    if (a.prototype !== b.prototype) return false;

    function isArguments(object) {
      return Object.prototype.toString.call(object) == '[object Arguments]';
    }

    
    
    if (isArguments(a)) {
      if (!isArguments(b)) {
        return false;
      }
      a = pSlice.call(a);
      b = pSlice.call(b);
      return _deepEqual(a, b);
    }
    try {
      var ka = Object.keys(a),
          kb = Object.keys(b),
          key, i;
    } catch (e) {
      return false;
    }
    
    
    if (ka.length != kb.length)
      return false;
    
    ka.sort();
    kb.sort();
    
    for (i = ka.length - 1; i >= 0; i--) {
      if (ka[i] != kb[i])
        return false;
    }
    
    
    for (i = ka.length - 1; i >= 0; i--) {
      key = ka[i];
      if (!this._deepEqual(a[key], b[key])) return false;
    }
    return true;
  },

  _expectedException : function Assert__expectedException(actual, expected) {
    if (!actual || !expected) {
      return false;
    }

    if (expected instanceof RegExp) {
      return expected.test(actual);
    } else if (actual instanceof expected) {
      return true;
    } else if (expected.call({}, actual) === true) {
      return true;
    } else if (actual.name === expected.name) {
      return true;
    }

    return false;
  },

  

















  _logFail: function Assert__logFail(aResult) {
    throw new errors.AssertionError(aResult.message,
                                    aResult.fileName,
                                    aResult.lineNumber,
                                    aResult.functionName,
                                    aResult.name);
  },

  















  _logPass: function Assert__logPass(aResult) {
    broker.pass({pass: aResult});
  },

  












  _test: function Assert__test(aCondition, aMessage, aDiagnosis) {
    let diagnosis = aDiagnosis || "";
    let message = aMessage || "";

    if (diagnosis)
      message = aMessage ? message + " - " + diagnosis : diagnosis;

    
    let frame = stack.findCallerFrame(Components.stack);

    let result = {
      'fileName'     : frame.filename.replace(/(.*)-> /, ""),
      'functionName' : frame.name,
      'lineNumber'   : frame.lineNumber,
      'message'      : message
    };

    
    if (aCondition) {
      this._logPass(result);
    }
    else {
      result.stack = Components.stack;
      this._logFail(result);
    }

    return aCondition;
  },

  






  pass: function Assert_pass(aMessage) {
    return this._test(true, aMessage, undefined);
  },

  








  fail: function Assert_fail(aMessage) {
    return this._test(false, aMessage, undefined);
  },

  










  ok: function Assert_ok(aValue, aMessage) {
    let condition = !!aValue;
    let diagnosis = "got '" + aValue + "'";

    return this._test(condition, aMessage, diagnosis);
  },

 












  equal: function Assert_equal(aValue, aExpected, aMessage) {
    let condition = (aValue === aExpected);
    let diagnosis = "'" + aValue + "' should equal '" + aExpected + "'";

    return this._test(condition, aMessage, diagnosis);
  },

 












  notEqual: function Assert_notEqual(aValue, aExpected, aMessage) {
    let condition = (aValue !== aExpected);
    let diagnosis = "'" + aValue + "' should not equal '" + aExpected + "'";

    return this._test(condition, aMessage, diagnosis);
  },

  












  deepEqual: function equal(aValue, aExpected, aMessage) {
    let condition = this._deepEqual(aValue, aExpected);
    try {
      var aValueString = JSON.stringify(aValue);
    } catch (e) {
      var aValueString = String(aValue);
    }
    try {
      var aExpectedString = JSON.stringify(aExpected);
    } catch (e) {
      var aExpectedString = String(aExpected);
    }

    let diagnosis = "'" + aValueString + "' should equal '" +
                    aExpectedString + "'";

    return this._test(condition, aMessage, diagnosis);
  },

  












  notDeepEqual: function notEqual(aValue, aExpected, aMessage) {
     let condition = !this._deepEqual(aValue, aExpected);
     try {
       var aValueString = JSON.stringify(aValue);
     } catch (e) {
       var aValueString = String(aValue);
     }
     try {
       var aExpectedString = JSON.stringify(aExpected);
     } catch (e) {
       var aExpectedString = String(aExpected);
     }

     let diagnosis = "'" + aValueString + "' should not equal '" +
                     aExpectedString + "'";

     return this._test(condition, aMessage, diagnosis);
  },

  












  match: function Assert_match(aString, aRegex, aMessage) {
    
    
    
    let pattern = flags = "";
    try {
      let matches = aRegex.toString().match(/\/(.*)\/(.*)/);

      pattern = matches[1];
      flags = matches[2];
    } catch (e) {
    }

    let regex = new RegExp(pattern, flags);
    let condition = (aString.match(regex) !== null);
    let diagnosis = "'" + regex + "' matches for '" + aString + "'";

    return this._test(condition, aMessage, diagnosis);
  },

  












  notMatch: function Assert_notMatch(aString, aRegex, aMessage) {
    
    
    
    let pattern = flags = "";
    try {
      let matches = aRegex.toString().match(/\/(.*)\/(.*)/);

      pattern = matches[1];
      flags = matches[2];
    } catch (e) {
    }

    let regex = new RegExp(pattern, flags);
    let condition = (aString.match(regex) === null);
    let diagnosis = "'" + regex + "' doesn't match for '" + aString + "'";

    return this._test(condition, aMessage, diagnosis);
  },


  












  throws : function Assert_throws(block, error, message) {
    return this._throws.apply(this, [true].concat(Array.prototype.slice.call(arguments)));
  },

  












  doesNotThrow : function Assert_doesNotThrow(block, error, message) {
    return this._throws.apply(this, [false].concat(Array.prototype.slice.call(arguments)));
  },

  





  _throws : function Assert__throws(shouldThrow, block, expected, message) {
    var actual;

    if (typeof expected === 'string') {
      message = expected;
      expected = null;
    }

    try {
      block();
    } catch (e) {
      actual = e;
    }

    message = (expected && expected.name ? ' (' + expected.name + ').' : '.') +
              (message ? ' ' + message : '.');

    if (shouldThrow && !actual) {
      return this._test(false, message, 'Missing expected exception');
    }

    if (!shouldThrow && this._expectedException(actual, expected)) {
      return this._test(false, message, 'Got unwanted exception');
    }

    if ((shouldThrow && actual && expected &&
        !this._expectedException(actual, expected)) || (!shouldThrow && actual)) {
      throw actual;
    }

    return this._test(true, message);
  },

  









  contain: function Assert_contain(aString, aPattern, aMessage) {
    let condition = (aString.indexOf(aPattern) !== -1);
    let diagnosis = "'" + aString + "' should contain '" + aPattern + "'";

    return this._test(condition, aMessage, diagnosis);
  },

  









  notContain: function Assert_notContain(aString, aPattern, aMessage) {
    let condition = (aString.indexOf(aPattern) === -1);
    let diagnosis = "'" + aString + "' should not contain '" + aPattern + "'";

    return this._test(condition, aMessage, diagnosis);
  },

  
















  waitFor: function Assert_waitFor(aCallback, aMessage, aTimeout, aInterval, aThisObject) {
    var timeout = aTimeout || 5000;
    var interval = aInterval || 100;

    var self = {
      timeIsUp: false,
      result: aCallback.call(aThisObject)
    };
    var deadline = Date.now() + timeout;

    function wait() {
      if (self.result !== true) {
        self.result = aCallback.call(aThisObject);
        self.timeIsUp = Date.now() > deadline;
      }
    }

    var hwindow = Services.appShell.hiddenDOMWindow;
    var timeoutInterval = hwindow.setInterval(wait, interval);
    var thread = Services.tm.currentThread;

    while (self.result !== true && !self.timeIsUp) {
      thread.processNextEvent(true);

      let type = typeof(self.result);
      if (type !== 'boolean')
        throw TypeError("waitFor() callback has to return a boolean" +
                        " instead of '" + type + "'");
    }

    hwindow.clearInterval(timeoutInterval);

    if (self.result !== true && self.timeIsUp) {
      aMessage = aMessage || arguments.callee.name + ": Timeout exceeded for '" + aCallback + "'";
      throw new errors.TimeoutError(aMessage);
    }

    broker.pass({'function':'assert.waitFor()'});
    return true;
  }
}


var Expect = function () {}

Expect.prototype = new Assert();

















Expect.prototype._logFail = function Expect__logFail(aResult) {
  broker.fail({fail: aResult});
}















Expect.prototype.waitFor = function Expect_waitFor(aCallback, aMessage, aTimeout, aInterval, aThisObject) {
  let condition = true;
  let message = aMessage;

  try {
    Assert.prototype.waitFor.apply(this, arguments);
  }
  catch (ex if ex instanceof errors.AssertionError) {
    message = ex.message;
    condition = false;
  }

  return this._test(condition, message);
}
