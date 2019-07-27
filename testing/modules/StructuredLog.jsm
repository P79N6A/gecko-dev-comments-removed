



"use strict";

this.EXPORTED_SYMBOLS = [
  "StructuredLogger"
];















this.StructuredLogger = function (name, dumpFun=dump, mutators=[]) {
  this.name = name;
  this._dumpFun = dumpFun;
  this._mutatorFuns = mutators;
  this._runningTests = new Set();
}





StructuredLogger.prototype.testStart = function (test) {
  this._runningTests.add(test);
  let data = {test: test};
  this._logData("test_start", data);
}

StructuredLogger.prototype.testStatus = function (test, subtest, status, expected="PASS",
                                                  message=null, stack=null, extra=null) {
  let data = {
    test: test,
    subtest: subtest,
    status: status,
  };

  if (expected != status && status != "SKIP") {
    data.expected = expected;
  }
  if (message !== null) {
    data.message = message;
  }
  if (stack !== null) {
    data.stack = stack;
  }
  if (extra !== null) {
    data.extra = extra;
  }

  this._logData("test_status", data);
}

StructuredLogger.prototype.testEnd = function (test, status, expected="OK", message=null,
                                               stack=null, extra=null) {
  let data = {test: test, status: status};

  if (expected != status && status != "SKIP") {
    data.expected = expected;
  }
  if (message !== null) {
    data.message = message;
  }
  if (stack !== null) {
    data.stack = stack;
  }
  if (extra !== null) {
    data.extra = extra;
  }

  if (!this._runningTests.has(test)) {
    this.error("Test \"" + test + "\" was ended more than once or never started. " +
               "Ended with this data: " + JSON.stringify(data));
    return;
  }

  this._runningTests.delete(test);
  this._logData("test_end", data);
}

StructuredLogger.prototype.suiteStart = function (tests, runinfo=null) {

  let data = {tests: tests};
  if (runinfo !== null) {
    data.runinfo = runinfo;
  }

  this._logData("suite_start", data);
};

StructuredLogger.prototype.suiteEnd = function () {
  this._logData("suite_end");
};






StructuredLogger.prototype.log = function (level, message, extra=null) {
  let data = {
    level: level,
    message: message,
  };

  if (extra !== null) {
    data.extra = extra;
    if ("stack" in extra) {
      data.stack = extra.stack;
    }
  }

  this._logData("log", data);
}

StructuredLogger.prototype.debug = function (message, extra=null) {
  this.log("DEBUG", message, extra);
}

StructuredLogger.prototype.info = function (message, extra=null) {
  this.log("INFO", message, extra);
}

StructuredLogger.prototype.warning = function (message, extra=null) {
  this.log("WARNING", message, extra);
}

StructuredLogger.prototype.error = function (message, extra=null) {
  this.log("ERROR", message, extra);
}

StructuredLogger.prototype.critical = function (message, extra=null) {
  this.log("CRITICAL", message, extra);
}

StructuredLogger.prototype._logData = function (action, data={}) {
  let allData = {
    action: action,
    time: Date.now(),
    thread: null,
    pid: null,
    source: this.name
  };

  for (let field in data) {
    allData[field] = data[field];
  }

  for (let fun of this._mutatorFuns) {
    fun(allData);
  }

  this._dumpMessage(allData);
};

StructuredLogger.prototype._dumpMessage = function (message) {
  this._dumpFun(JSON.stringify(message));
}
