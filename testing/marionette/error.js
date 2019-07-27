



"use strict";

const {results: Cr, utils: Cu} = Components;

const errors = [
  "ElementNotAccessibleError",
  "ElementNotVisibleError",
  "FrameSendFailureError",
  "FrameSendNotInitializedError",
  "InvalidArgumentError",
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
  "StaleElementReferenceError",
  "TimeoutError",
  "UnableToSetCookieError",
  "UnknownCommandError",
  "UnknownError",
  "UnsupportedOperationError",
  "WebDriverError",
];

this.EXPORTED_SYMBOLS = ["error"].concat(errors);














const XPCOM_EXCEPTIONS = [];
{
  for (let prop in Cr) {
    XPCOM_EXCEPTIONS.push(Cr[prop]);
  }
}

this.error = {};

error.toJSON = function(err) {
  return {
    message: err.message,
    stacktrace: err.stack || null,
    status: err.status
  };
};




error.isSuccess = status => status === "success";












error.isError = function(val) {
  if (val === null || typeof val != "object") {
    return false;
  } else if ("result" in val && val.result in XPCOM_EXCEPTIONS) {
    return true;
  } else {
    return Object.getPrototypeOf(val) == "Error";
  }
};




error.isWebDriverError = function(obj) {
  return error.isError(obj) &&
      ("name" in obj && errors.indexOf(obj.name) > 0);
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
};
WebDriverError.prototype = Object.create(Error.prototype);

this.ElementNotAccessibleError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "ElementNotAccessibleError";
  this.status = "element not accessible";
};
ElementNotAccessibleError.prototype = Object.create(WebDriverError.prototype);

this.ElementNotVisibleError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "ElementNotVisibleError";
  this.status = "element not visible";
};
ElementNotVisibleError.prototype = Object.create(WebDriverError.prototype);

this.FrameSendFailureError = function(frame) {
  this.message = "Error sending message to frame (NS_ERROR_FAILURE)";
  WebDriverError.call(this, this.message);
  this.name = "FrameSendFailureError";
  this.status = "frame send failure error";
  this.frame = frame;
  this.errMsg = `${this.message} ${this.frame}; frame not responding.`;
};
FrameSendFailureError.prototype = Object.create(WebDriverError.prototype);

this.FrameSendNotInitializedError = function(frame) {
  this.message = "Error sending message to frame (NS_ERROR_NOT_INITIALIZED)";
  WebDriverError.call(this, this.message);
  this.name = "FrameSendNotInitializedError";
  this.status = "frame send not initialized error";
  this.frame = frame;
  this.errMsg = `${this.message} ${this.frame}; frame has closed.`;
};
FrameSendNotInitializedError.prototype = Object.create(WebDriverError.prototype);

this.InvalidArgumentError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "InvalidArgumentError";
  this.status = "invalid argument";
};
InvalidArgumentError.prototype = Object.create(WebDriverError.prototype);

this.InvalidElementStateError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "InvalidElementStateError";
  this.status = "invalid element state";
};
InvalidElementStateError.prototype = Object.create(WebDriverError.prototype);

this.InvalidSelectorError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "InvalidSelectorError";
  this.status = "invalid selector";
};
InvalidSelectorError.prototype = Object.create(WebDriverError.prototype);

this.InvalidSessionIdError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "InvalidSessionIdError";
  this.status = "invalid session id";
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
  this.stack = trace;
};
JavaScriptError.prototype = Object.create(WebDriverError.prototype);

this.NoAlertOpenError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "NoAlertOpenError";
  this.status = "no such alert";
}
NoAlertOpenError.prototype = Object.create(WebDriverError.prototype);

this.NoSuchElementError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "NoSuchElementError";
  this.status = "no such element";
};
NoSuchElementError.prototype = Object.create(WebDriverError.prototype);

this.NoSuchFrameError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "NoSuchFrameError";
  this.status = "no such frame";
};
NoSuchFrameError.prototype = Object.create(WebDriverError.prototype);

this.NoSuchWindowError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "NoSuchWindowError";
  this.status = "no such window";
};
NoSuchWindowError.prototype = Object.create(WebDriverError.prototype);

this.ScriptTimeoutError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "ScriptTimeoutError";
  this.status = "script timeout";
};
ScriptTimeoutError.prototype = Object.create(WebDriverError.prototype);

this.SessionNotCreatedError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "SessionNotCreatedError";
  this.status = "session not created";
};
SessionNotCreatedError.prototype = Object.create(WebDriverError.prototype);

this.StaleElementReferenceError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "StaleElementReferenceError";
  this.status = "stale element reference";
};
StaleElementReferenceError.prototype = Object.create(WebDriverError.prototype);

this.TimeoutError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "TimeoutError";
  this.status = "timeout";
};
TimeoutError.prototype = Object.create(WebDriverError.prototype);

this.UnableToSetCookieError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "UnableToSetCookieError";
  this.status = "unable to set cookie";
};
UnableToSetCookieError.prototype = Object.create(WebDriverError.prototype);

this.UnknownCommandError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "UnknownCommandError";
  this.status = "unknown command";
};
UnknownCommandError.prototype = Object.create(WebDriverError.prototype);

this.UnknownError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "UnknownError";
  this.status = "unknown error";
};
UnknownError.prototype = Object.create(WebDriverError.prototype);

this.UnsupportedOperationError = function(msg) {
  WebDriverError.call(this, msg);
  this.name = "UnsupportedOperationError";
  this.status = "unsupported operation";
};
UnsupportedOperationError.prototype = Object.create(WebDriverError.prototype);
