











"use strict";

this.EXPORTED_SYMBOLS = [
  "Assert"
];

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");








let Assert = this.Assert = function(reporterFunc) {
  if (reporterFunc)
    this.setReporter(reporterFunc);
};

function instanceOf(object, type) {
  return Object.prototype.toString.call(object) == "[object " + type + "]";
}

function replacer(key, value) {
  if (value === undefined) {
    return "" + value;
  }
  if (typeof value === "number" && (isNaN(value) || !isFinite(value))) {
    return value.toString();
  }
  if (typeof value === "function" || instanceOf(value, "RegExp")) {
    return value.toString();
  }
  return value;
}

const kTruncateLength = 128;

function truncate(text, newLength = kTruncateLength) {
  if (typeof text == "string") {
    return text.length < newLength ? text : text.slice(0, newLength);
  } else {
    return text;
  }
}

function getMessage(error, prefix = "") {
  let actual, expected;
  
  
  try {
    actual = JSON.stringify(error.actual, replacer);
  } catch (ex) {
    actual = Object.prototype.toString.call(error.actual);
  }
  try {
    expected = JSON.stringify(error.expected, replacer);
  } catch (ex) {
    expected = Object.prototype.toString.call(error.expected);
  }
  let message = prefix;
  if (error.operator) {
    message += (prefix ? " - " : "") + truncate(actual) + " " + error.operator +
               " " + truncate(expected);
  }
  return message;
}

















Assert.AssertionError = function(options) {
  this.name = "AssertionError";
  this.actual = options.actual;
  this.expected = options.expected;
  this.operator = options.operator;
  this.message = getMessage(this, options.message);
  
  let stack = Components.stack;
  do {
    stack = stack.asyncCaller || stack.caller;
  } while(stack.filename && stack.filename.contains("Assert.jsm"))
  this.stack = stack;
};


Assert.AssertionError.prototype = Object.create(Error.prototype, {
  constructor: {
    value: Assert.AssertionError,
    enumerable: false,
    writable: true,
    configurable: true
  }
});

let proto = Assert.prototype;

proto._reporter = null;






















proto.setReporter = function(reporterFunc) {
  this._reporter = reporterFunc;
};




























proto.report = function(failed, actual, expected, message, operator) {
  let err = new Assert.AssertionError({
    message: message,
    actual: actual,
    expected: expected,
    operator: operator
  });
  if (!this._reporter) {
    
    if (failed) {
      throw err;
    }
  } else {
    this._reporter(failed ? err : null, err.message, err.stack);
  }
};













proto.ok = function(value, message) {
  this.report(!value, value, true, message, "==");
};












proto.equal = function equal(actual, expected, message) {
  this.report(actual != expected, actual, expected, message, "==");
};












proto.notEqual = function notEqual(actual, expected, message) {
  this.report(actual == expected, actual, expected, message, "!=");
};

















proto.deepEqual = function deepEqual(actual, expected, message) {
  this.report(!_deepEqual(actual, expected), actual, expected, message, "deepEqual");
};

function _deepEqual(actual, expected) {
  
  if (actual === expected) {
    return true;
  
  
  } else if (instanceOf(actual, "Date") && instanceOf(expected, "Date")) {
    if (isNaN(actual.getTime()) && isNaN(expected.getTime()))
      return true;
    return actual.getTime() === expected.getTime();
  
  
  
  } else if (instanceOf(actual, "RegExp") && instanceOf(expected, "RegExp")) {
    return actual.source === expected.source &&
           actual.global === expected.global &&
           actual.multiline === expected.multiline &&
           actual.lastIndex === expected.lastIndex &&
           actual.ignoreCase === expected.ignoreCase;
  
  
  } else if (typeof actual != "object" && typeof expected != "object") {
    return actual == expected;
  
  
  
  
  
  
  } else {
    return objEquiv(actual, expected);
  }
}

function isUndefinedOrNull(value) {
  return value === null || value === undefined;
}

function isArguments(object) {
  return instanceOf(object, "Arguments");
}

function objEquiv(a, b) {
  if (isUndefinedOrNull(a) || isUndefinedOrNull(b)) {
    return false;
  }
  
  if (a.prototype !== b.prototype) {
    return false;
  }
  
  
  if (isArguments(a)) {
    if (!isArguments(b)) {
      return false;
    }
    a = pSlice.call(a);
    b = pSlice.call(b);
    return _deepEqual(a, b);
  }
  let ka, kb, key, i;
  try {
    ka = Object.keys(a);
    kb = Object.keys(b);
  } catch (e) {
    
    return false;
  }
  
  
  if (ka.length != kb.length)
    return false;
  
  ka.sort();
  kb.sort();
  
  
  for (i = ka.length - 1; i >= 0; i--) {
    key = ka[i];
    if (!_deepEqual(a[key], b[key])) {
      return false;
    }
  }
  return true;
}












proto.notDeepEqual = function notDeepEqual(actual, expected, message) {
  this.report(_deepEqual(actual, expected), actual, expected, message, "notDeepEqual");
};












proto.strictEqual = function strictEqual(actual, expected, message) {
  this.report(actual !== expected, actual, expected, message, "===");
};












proto.notStrictEqual = function notStrictEqual(actual, expected, message) {
  this.report(actual === expected, actual, expected, message, "!==");
};

function expectedException(actual, expected) {
  if (!actual || !expected) {
    return false;
  }

  if (instanceOf(expected, "RegExp")) {
    return expected.test(actual);
  } else if (actual instanceof expected) {
    return true;
  } else if (expected.call({}, actual) === true) {
    return true;
  }

  return false;
}












proto.throws = function(block, expected, message) {
  let actual;

  if (typeof expected === "string") {
    message = expected;
    expected = null;
  }

  try {
    block();
  } catch (e) {
    actual = e;
  }

  message = (expected && expected.name ? " (" + expected.name + ")." : ".") +
            (message ? " " + message : ".");

  if (!actual) {
    this.report(true, actual, expected, "Missing expected exception" + message);
  }

  if ((actual && expected && !expectedException(actual, expected))) {
    throw actual;
  }

  this.report(false, expected, expected, message);
};












proto.rejects = function(promise, expected, message) {
  return new Promise((resolve, reject) => {
    if (typeof expected === "string") {
      message = expected;
      expected = null;
    }
    return promise.then(
      () => this.report(true, null, expected, "Missing expected exception " + message),
      err => {
        if (expected && !expectedException(err, expected)) {
          reject(err);
          return;
        }
        this.report(false, err, expected, message);
        resolve();
      }
    ).then(null, reject);
  });
};
