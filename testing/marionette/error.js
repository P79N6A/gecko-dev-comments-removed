



"use strict";

const {utils: Cu} = Components;

const errors = [
  "ElementNotVisibleError",
  "FrameSendFailureError",
  "FrameSendNotInitializedError",
  "IllegalArgumentError",
  "InvalidElementStateError",
  "InvalidSelectorError",
  "InvalidSessionIdError",
  "JavaScriptError",
  "NoAlertOpenError",
  "NoSuchElementError",
  "NoSuchFrameError",
  "NoSuchWindowError",
  "ScriptTimeoutError",
  "SessionNotCreatedError",
  "TimeoutError",
  "UnknownCommandError",
  "UnknownError",
  "UnsupportedOperationError",
  "WebDriverError",
];

this.EXPORTED_SYMBOLS = ["error"].concat(errors);

this.error = {};

error.toJSON = function(err) {
  return {
    message: err.message,
    stacktrace: err.stack || null,
    status: err.code
  };
};




error.byCode = n => lookup.get(n);




error.isSuccess = code => code === 0;








let isOldStyleError = function(obj) {
  return typeof obj == "object" &&
    ["message", "code", "stack"].every(c => obj.hasOwnProperty(c));
}







error.isError = function(obj) {
  if (obj === null || typeof obj != "object") {
    return false;
  
  
  
  
  } else if ("result" in obj) {
    return true;
  } else {
    return Object.getPrototypeOf(obj) == "Error" || isOldStyleError(obj);
  }
};




error.isWebDriverError = function(obj) {
  return error.isError(obj) &&
    (("name" in obj && errors.indexOf(obj.name) > 0) ||
      isOldStyleError(obj));
};





error.report = function(err) {
  let msg = `Marionette threw an error: ${error.stringify(err)}`;
  dump(msg + "\n");
  if (Cu.reportError) {
    Cu.reportError(msg);
  }
};




error.stringify = function(err) {
  try {
    let s = err.toString();
    if ("stack" in err) {
      s += "\n" + err.stack;
    }
    return s;
  } catch (e) {
    return "<unprintable error>";
  }
};






this.WebDriverError = function(msg) {
  Error.call(this, msg);
  this.name = "WebDriverError";
  this.message = msg;
  this.status = "webdriver error";
  this.code = 500;  
};
WebDriverError.prototype = Object.create(Error.prototype);

this.ElementNotVisibleError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "ElementNotVisibleError";
  this.status = "element not visible";
  this.code = 11;
};
ElementNotVisibleError.prototype = Object.create(WebDriverError.prototype);

this.FrameSendFailureError = function(frame) {
  this.message = "Error sending message to frame (NS_ERROR_FAILURE)";
  WebDriverError.call(this, this.message);
  this.name = "FrameSendFailureError";
  this.status = "frame send failure error";
  this.code = 55;
  this.frame = frame;
  this.errMsg = `${this.message} ${this.frame}; frame not responding.`;
};
FrameSendFailureError.prototype = Object.create(WebDriverError.prototype);

this.FrameSendNotInitializedError = function(frame) {
  this.message = "Error sending message to frame (NS_ERROR_NOT_INITIALIZED)";
  WebDriverError.call(this, this.message);
  this.name = "FrameSendNotInitializedError";
  this.status = "frame send not initialized error";
  this.code = 54;
  this.frame = frame;
  this.errMsg = `${this.message} ${this.frame}; frame has closed.`;
};
FrameSendNotInitializedError.prototype = Object.create(WebDriverError.prototype);

this.IllegalArgumentError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "IllegalArgumentError";
  this.status = "illegal argument";
  this.code = 13;  
};
IllegalArgumentError.prototype = Object.create(WebDriverError.prototype);

this.InvalidElementStateError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "InvalidElementStateError";
  this.status = "invalid element state";
  this.code = 12;
};
InvalidElementStateError.prototype = Object.create(WebDriverError.prototype);

this.InvalidSelectorError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "InvalidSelectorError";
  this.status = "invalid selector";
  this.code = 32;
};
InvalidSelectorError.prototype = Object.create(WebDriverError.prototype);

this.InvalidSessionIdError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "InvalidSessionIdError";
  this.status = "invalid session id";
  this.code = 13;
};
InvalidSessionIdError.prototype = Object.create(WebDriverError.prototype);


















this.JavaScriptError = function(err, fnName, file, line, script) {
  let msg = String(err);
  let trace = "";

  if (fnName && line) {
    trace += `${fnName} @${file}`;
    if (line) {
      trace += `, line ${line}`;
    }
  }

  if (typeof err == "object" && "name" in err && "stack" in err) {
    let jsStack = err.stack.split("\n");
    let match = jsStack[0].match(/:(\d+):\d+$/);
    let jsLine = match ? parseInt(match[1]) : 0;
    if (script) {
      let src = script.split("\n")[jsLine];
      trace += "\n" +
        "inline javascript, line " + jsLine + "\n" +
        "src: \"" + src + "\"";
    }
  }

  WebDriverError.call(this, msg);
  this.name = "JavaScriptError";
  this.status = "javascript error";
  this.code = 17;
  this.stack = trace;
};
JavaScriptError.prototype = Object.create(WebDriverError.prototype);

this.NoAlertOpenError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "NoAlertOpenError";
  this.status = "no such alert";
  this.code = 27;
}
NoAlertOpenError.prototype = Object.create(WebDriverError.prototype);

this.NoSuchElementError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "NoSuchElementError";
  this.status = "no such element";
  this.code = 7;
};
NoSuchElementError.prototype = Object.create(WebDriverError.prototype);

this.NoSuchFrameError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "NoSuchFrameError";
  this.status = "no such frame";
  this.code = 8;
};
NoSuchFrameError.prototype = Object.create(WebDriverError.prototype);

this.NoSuchWindowError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "NoSuchWindowError";
  this.status = "no such window";
  this.code = 23;
};
NoSuchWindowError.prototype = Object.create(WebDriverError.prototype);

this.ScriptTimeoutError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "ScriptTimeoutError";
  this.status = "script timeout";
  this.code = 28;
};
ScriptTimeoutError.prototype = Object.create(WebDriverError.prototype);

this.SessionNotCreatedError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "SessionNotCreatedError";
  this.status = "session not created";
  
  this.code = 71;
}
SessionNotCreatedError.prototype = Object.create(WebDriverError.prototype);

this.TimeoutError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "TimeoutError";
  this.status = "timeout";
  this.code = 21;
};
TimeoutError.prototype = Object.create(WebDriverError.prototype);

this.UnknownCommandError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "UnknownCommandError";
  this.status = "unknown command";
  this.code = 9;
};
UnknownCommandError.prototype = Object.create(WebDriverError.prototype);

this.UnknownError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "UnknownError";
  this.status = "unknown error";
  this.code = 13;
};
UnknownError.prototype = Object.create(WebDriverError.prototype);

this.UnsupportedOperationError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "UnsupportedOperationError";
  this.status = "unsupported operation";
  this.code = 405;
};
UnsupportedOperationError.prototype = Object.create(WebDriverError.prototype);

const errorObjs = [
  this.ElementNotVisibleError,
  this.FrameSendFailureError,
  this.FrameSendNotInitializedError,
  this.IllegalArgumentError,
  this.InvalidElementStateError,
  this.InvalidSelectorError,
  this.InvalidSessionIdError,
  this.JavaScriptError,
  this.NoAlertOpenError,
  this.NoSuchElementError,
  this.NoSuchFrameError,
  this.NoSuchWindowError,
  this.ScriptTimeoutError,
  this.SessionNotCreatedError,
  this.TimeoutError,
  this.UnknownCommandError,
  this.UnknownError,
  this.UnsupportedOperationError,
  this.WebDriverError,
];
const lookup = new Map(errorObjs.map(err => [new err().code, err]));
