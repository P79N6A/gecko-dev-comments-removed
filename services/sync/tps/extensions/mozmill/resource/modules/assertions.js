




































var mozmillFrame = {};
Cu.import('resource://mozmill/modules/frame.js', mozmillFrame);






var assertions = exports;



var Expect = function() {}

Expect.prototype = {

  















  _logFail: function Expect__logFail(aResult) {
    mozmillFrame.events.fail({fail: aResult});
  },

  















  _logPass: function Expect__logPass(aResult) {
    mozmillFrame.events.pass({pass: aResult});
  },

  










  _test: function Expect__test(aCondition, aMessage, aDiagnosis) {
    let diagnosis = aDiagnosis || "";
    let message = aMessage || "";

    if (diagnosis)
      message = aMessage ? message + " - " + diagnosis : diagnosis;

    
    let frame = Components.stack;
    let result = {
      'fileName'   : frame.filename.replace(/(.*)-> /, ""),
      'function'   : frame.name,
      'lineNumber' : frame.lineNumber,
      'message'    : message
    };

    
    if (aCondition)
      this._logPass(result);
    else
      this._logFail(result);

    return aCondition;
  },

  






  pass: function Expect_pass(aMessage) {
    return this._test(true, aMessage, undefined);
  },

  






  fail: function Expect_fail(aMessage) {
    return this._test(false, aMessage, undefined);
  },

  








  ok: function Expect_ok(aValue, aMessage) {
    let condition = !!aValue;
    let diagnosis = "got '" + aValue + "'";

    return this._test(condition, aMessage, diagnosis);
  },

  










  equal: function Expect_equal(aValue, aExpected, aMessage) {
    let condition = (aValue === aExpected);
    let diagnosis = "got '" + aValue + "', expected '" + aExpected + "'";

    return this._test(condition, aMessage, diagnosis);
  },

  










  notEqual: function Expect_notEqual(aValue, aExpected, aMessage) {
    let condition = (aValue !== aExpected);
    let diagnosis = "got '" + aValue + "', not expected '" + aExpected + "'";

    return this._test(condition, aMessage, diagnosis);
  },

  










  match: function Expect_match(aString, aRegex, aMessage) {
    
    
    
    let pattern = flags = "";
    try {
      let matches = aRegex.toString().match(/\/(.*)\/(.*)/);

      pattern = matches[1];
      flags = matches[2];
    }
    catch (ex) {
    }

    let regex = new RegExp(pattern, flags);
    let condition = (aString.match(regex) !== null);
    let diagnosis = "'" + regex + "' matches for '" + aString + "'";

    return this._test(condition, aMessage, diagnosis);
  },

  










  notMatch: function Expect_notMatch(aString, aRegex, aMessage) {
    
    
    
    let pattern = flags = "";
    try {
      let matches = aRegex.toString().match(/\/(.*)\/(.*)/);

      pattern = matches[1];
      flags = matches[2];
    }
    catch (ex) {
    }

    let regex = new RegExp(pattern, flags);
    let condition = (aString.match(regex) === null);
    let diagnosis = "'" + regex + "' doesn't match for '" + aString + "'";

    return this._test(condition, aMessage, diagnosis);
  },


  










  throws : function Expect_throws(block, error, message) {
    return this._throws.apply(this, [true].concat(Array.prototype.slice.call(arguments)));
  },

  










  doesNotThrow : function Expect_doesNotThrow(block, error, message) {
    return this._throws.apply(this, [false].concat(Array.prototype.slice.call(arguments)));
  },

  





  _throws : function Expect__throws(shouldThrow, block, expected, message) {
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

  _expectedException : function Expect__expectedException(actual, expected) {
    if (!actual || !expected) {
      return false;
    }

    if (expected instanceof RegExp) {
      return expected.test(actual);
    } else if (actual instanceof expected) {
      return true;
    } else if (expected.call({}, actual) === true) {
      return true;
    }

    return false;
  }
}






function AssertionError(message, fileName, lineNumber) {
  var err = new Error();
  if (err.stack) {
    this.stack = err.stack;
  }
  this.message = message === undefined ? err.message : message;
  this.fileName = fileName === undefined ? err.fileName : fileName;
  this.lineNumber = lineNumber === undefined ? err.lineNumber : lineNumber;
};
AssertionError.prototype = new Error();
AssertionError.prototype.constructor = AssertionError;
AssertionError.prototype.name = 'AssertionError';


var Assert = function() {}

Assert.prototype = new Expect();

Assert.prototype.AssertionError = AssertionError;

























Assert.prototype._logFail = function Assert__logFail(aResult) {
  throw new AssertionError(aResult);
}



assertions.Expect = Expect;
assertions.Assert = Assert;
