







"use strict";

function getElement(id) {
    return ((typeof(id) == "string") ?
        document.getElementById(id) : id);
}

this.$ = this.getElement;

function contentDispatchEvent(type, data, sync) {
  if (typeof(data) == "undefined") {
    data = {};
  }

  var event = new CustomEvent("contentEvent", {
    bubbles: true,
    detail: {
      "sync": sync,
      "type": type,
      "data": JSON.stringify(data)
    }
  });
  document.dispatchEvent(event);
}

function contentAsyncEvent(type, data) {
  contentDispatchEvent(type, data, 0);
}


function extend(obj,  skip) {
    
    
    if (!skip) {
        skip = 0;
    }
    if (obj) {
        var l = obj.length;
        var ret = [];
        for (var i = skip; i < l; i++) {
            ret.push(obj[i]);
        }
    }
    return ret;
}

function flattenArguments(lst) {
    var res = [];
    var args = extend(arguments);
    while (args.length) {
        var o = args.shift();
        if (o && typeof(o) == "object" && typeof(o.length) == "number") {
            for (var i = o.length - 1; i >= 0; i--) {
                args.unshift(o[i]);
            }
        } else {
            res.push(o);
        }
    }
    return res;
}





this.StructuredFormatter = function() {
    this.testStartTimes = {};
};

StructuredFormatter.prototype.log = function(message) {
  return message.message;
};

StructuredFormatter.prototype.suite_start = function(message) {
    this.suiteStartTime = message.time;
    return "SUITE-START | Running " +  message.tests.length + " tests";
};

StructuredFormatter.prototype.test_start = function(message) {
    this.testStartTimes[message.test] = new Date().getTime();
    return "TEST-START | " + message.test;
};

StructuredFormatter.prototype.test_status = function(message) {
    var statusInfo = message.test + " | " + message.subtest +
                    (message.message ? " | " + message.message : "");
    if (message.expected) {
        return "TEST-UNEXPECTED-" + message.status + " | " + statusInfo +
               " - expected: " + message.expected;
    } else {
        return "TEST-" + message.status + " | " + statusInfo;
    }
};

StructuredFormatter.prototype.test_end = function(message) {
    var startTime = this.testStartTimes[message.test];
    delete this.testStartTimes[message.test];
    var statusInfo = message.test + (message.message ? " | " + String(message.message) : "");
    var result;
    if (message.expected) {
        result = "TEST-UNEXPECTED-" + message.status + " | " + statusInfo +
                 " - expected: " + message.expected;
    } else {
        return "TEST-" + message.status + " | " + statusInfo;
    }
    result = " | took " + message.time - startTime + "ms";
};

StructuredFormatter.prototype.suite_end = function(message) {
    return "SUITE-END | took " + message.time - this.suiteStartTime + "ms";
};






var VALID_ACTIONS = ['suite_start', 'suite_end', 'test_start', 'test_end', 'test_status', 'process_output', 'log'];

var LOG_DELIMITER = String.fromCharCode(0xe175) + String.fromCharCode(0xee31) + String.fromCharCode(0x2c32) + String.fromCharCode(0xacbf);

function StructuredLogger(name) {
    this.name = name;
    this.testsStarted = [];
    this.interactiveDebugger = false;
    this.structuredFormatter = new StructuredFormatter();

    

    this.testStart = function(test) {
        var data = {test: test};
        this._logData("test_start", data);
    };

    this.testStatus = function(test, subtest, status, expected="PASS", message=null) {
        
        if (subtest === null || subtest === undefined) {
            subtest = "undefined assertion name";
        }

        var data = {test: test, subtest: subtest, status: status};

        if (message) {
            data.message = String(message);
        }
        if (expected != status && status != 'SKIP') {
            data.expected = expected;
        }

        this._logData("test_status", data);
    };

    this.testEnd = function(test, status, expected="OK", message=null, extra=null) {
        var data = {test: test, status: status};

        if (message !== null) {
            data.message = String(message);
        }
        if (expected != status) {
            data.expected = expected;
        }
        if (extra !== null) {
            data.extra = extra;
        }

        this._logData("test_end", data);
    };

    this.suiteStart = function(tests, runinfo) {
        runinfo = typeof runinfo !== "undefined" ? runinfo : null;

        var data = {tests: tests};
        if (runinfo !== null) {
            data.runinfo = runinfo;
        }

        this._logData("suite_start", data);
    };

    this.suiteEnd = function() {
        this._logData("suite_end");
    };

    this.testStart = function(test) {
        this.testsStarted.push(test);
        var data = {test: test};
        this._logData("test_start", data);
    };

    

    this._log = function(level, message) {
        
        message = String(message);
        var data = {level: level, message: message};
        this._logData('log', data);
    };

    this.debug = function(message) {
        this._log('DEBUG', message);
    };

    this.info = function(message) {
        this._log('INFO', message);
    };

    this.warning = function(message) {
        this._log('WARNING', message);
    };

    this.error = function(message) {
        this._log("ERROR", message);
    };

    this.critical = function(message) {
        this._log('CRITICAL', message);
    };

    

    this.deactivateBuffering = function() {
        this._logData("buffering_off");
    };
    this.activateBuffering = function() {
        this._logData("buffering_on");
    };

    

    this._logData = function(action, data) {
        data = typeof data !== "undefined" ? data : null;

        if (data === null) {
            data = {};
        }

        var allData = {action: action,
                       time: new Date().getTime(),
                       thread: "",
                       
                       
                       
                       js_source: "TestRunner",
                       pid: null,
                       source: this.name};

        for (var attrname in data) {
            allData[attrname] = data[attrname];
        }

        this._dumpMessage(allData);
    };

    this._dumpMessage = function(message) {
        var str;
        if (this.interactiveDebugger && !message.action.startsWith("buffering_")) {
            str = this.structuredFormatter[message.action](message);
        } else {
            str = LOG_DELIMITER + JSON.stringify(message) + LOG_DELIMITER;
        }

        
        if (Object.keys(LogController.listeners).length !== 0) {
            LogController.log(str);
        } else {
            dump('\n' + str + '\n');
        }

        
        if (message.expected || (message.level && message.level === "ERROR")) {
            TestRunner.failureHandler();
        }
    };

    
    this.validMessage = function(message) {
        return message.action !== undefined && VALID_ACTIONS.indexOf(message.action) >= 0;
    };
}









var TestRunner = {};
TestRunner.logEnabled = false;
TestRunner._currentTest = 0;
TestRunner._lastTestFinished = -1;
TestRunner._loopIsRestarting = false;
TestRunner.currentTestURL = "";
TestRunner.originalTestURL = "";
TestRunner._urls = [];
TestRunner._lastAssertionCount = 0;
TestRunner._expectedMinAsserts = 0;
TestRunner._expectedMaxAsserts = 0;

TestRunner.timeout = 5 * 60 * 1000; 
TestRunner.maxTimeouts = 4; 
TestRunner.runSlower = false;
TestRunner.dumpOutputDirectory = "";
TestRunner.dumpAboutMemoryAfterTest = false;
TestRunner.dumpDMDAfterTest = false;
TestRunner.slowestTestTime = 0;
TestRunner.slowestTestURL = "";

TestRunner._expectingProcessCrash = false;




TestRunner._numTimeouts = 0;
TestRunner._currentTestStartTime = new Date().valueOf();
TestRunner._timeoutFactor = 1;

TestRunner._checkForHangs = function() {
  function reportError(win, msg) {
    if ("SimpleTest" in win) {
      win.SimpleTest.ok(false, msg);
    } else if ("W3CTest" in win) {
      win.W3CTest.logFailure(msg);
    }
  }

  function killTest(win) {
    if ("SimpleTest" in win) {
      win.SimpleTest.finish();
    } else if ("W3CTest" in win) {
      win.W3CTest.timeout();
    }
  }

  if (TestRunner._currentTest < TestRunner._urls.length) {
    var runtime = new Date().valueOf() - TestRunner._currentTestStartTime;
    if (runtime >= TestRunner.timeout * TestRunner._timeoutFactor) {
      var frameWindow = $('testframe').contentWindow.wrappedJSObject ||
                          $('testframe').contentWindow;
      
      reportError(frameWindow, "Test timed out.");

      
      
      if (++TestRunner._numTimeouts >= TestRunner.maxTimeouts) {
        TestRunner._haltTests = true;

        TestRunner.currentTestURL = "(SimpleTest/TestRunner.js)";
        reportError(frameWindow, TestRunner.maxTimeouts + " test timeouts, giving up.");
        var skippedTests = TestRunner._urls.length - TestRunner._currentTest;
        reportError(frameWindow, "Skipping " + skippedTests + " remaining tests.");
      }

      
      
      setTimeout(function delayedKillTest() { killTest(frameWindow); }, 1000);

      if (TestRunner._haltTests)
        return;
    }

    setTimeout(TestRunner._checkForHangs, 30000);
  }
}

TestRunner.requestLongerTimeout = function(factor) {
    TestRunner._timeoutFactor = factor;
}




TestRunner.repeat = 0;
TestRunner._currentLoop = 1;

TestRunner.expectAssertions = function(min, max) {
    if (typeof(max) == "undefined") {
        max = min;
    }
    if (typeof(min) != "number" || typeof(max) != "number" ||
        min < 0 || max < min) {
        throw "bad parameter to expectAssertions";
    }
    TestRunner._expectedMinAsserts = min;
    TestRunner._expectedMaxAsserts = max;
}




TestRunner.onComplete = null;




TestRunner._failedTests = {};
TestRunner._failureFile = "";

TestRunner.addFailedTest = function(testName) {
    if (TestRunner._failedTests[testName] == undefined) {
        TestRunner._failedTests[testName] = "";
    }
};

TestRunner.setFailureFile = function(fileName) {
    TestRunner._failureFile = fileName;
}

TestRunner.generateFailureList = function () {
    if (TestRunner._failureFile) {
        var failures = new SpecialPowersLogger(TestRunner._failureFile);
        failures.log(JSON.stringify(TestRunner._failedTests));
        failures.close();
    }
};




TestRunner.structuredLogger = new StructuredLogger('mochitest');

TestRunner.log = function(msg) {
    if (TestRunner.logEnabled) {
        TestRunner.structuredLogger.info(msg);
    } else {
        dump(msg + "\n");
    }
};

TestRunner.error = function(msg) {
    if (TestRunner.logEnabled) {
        TestRunner.structuredLogger.error(msg);
    } else {
        dump(msg + "\n");
        TestRunner.failureHandler();
    }
};

TestRunner.failureHandler = function() {
    if (TestRunner.runUntilFailure) {
      TestRunner._haltTests = true;
    }

    if (TestRunner.debugOnFailure) {
      
      
      debugger;
    }
};




TestRunner._toggle = function(el) {
    if (el.className == "noshow") {
        el.className = "";
        el.style.cssText = "";
    } else {
        el.className = "noshow";
        el.style.cssText = "width:0px; height:0px; border:0px;";
    }
};




TestRunner._makeIframe = function (url, retry) {
    var iframe = $('testframe');
    if (url != "about:blank" &&
        (("hasFocus" in document && !document.hasFocus()) ||
         ("activeElement" in document && document.activeElement != iframe))) {

        contentAsyncEvent("Focus");
        window.focus();
        SpecialPowers.focus();
        iframe.focus();
        if (retry < 3) {
            window.setTimeout('TestRunner._makeIframe("'+url+'", '+(retry+1)+')', 1000);
            return;
        }

        TestRunner.structuredLogger.info("Error: Unable to restore focus, expect failures and timeouts.");
    }
    window.scrollTo(0, $('indicator').offsetTop);
    iframe.src = url;
    iframe.name = url;
    iframe.width = "500";
    return iframe;
};






TestRunner.getLoadedTestURL = function () {
    var prefix = "";
    
    if ($('testframe').contentWindow.location.protocol == "chrome:") {
      prefix = "chrome://mochitests";
    }
    return prefix + $('testframe').contentWindow.location.pathname;
};

TestRunner.setParameterInfo = function (params) {
    this._params = params;
};

TestRunner.getParameterInfo = function() {
    return this._params;
};







TestRunner.runTests = function () {
    TestRunner.structuredLogger.info("SimpleTest START");
    TestRunner.originalTestURL = $("current-test").innerHTML;

    SpecialPowers.registerProcessCrashObservers();

    TestRunner._urls = flattenArguments(arguments);

    var singleTestRun = this._urls.length <= 1 && TestRunner.repeat <= 1;
    TestRunner.showTestReport = singleTestRun;
    var frame = $('testframe');
    frame.src = "";
    if (singleTestRun) {
        
        var body = document.getElementsByTagName("body")[0];
        body.setAttribute("singletest", "true");
        frame.removeAttribute("scrolling");
    }
    TestRunner._checkForHangs();
    TestRunner.runNextTest();
};





TestRunner.resetTests = function(listURLs) {
  TestRunner._currentTest = 0;
  
  $("current-test").innerHTML = TestRunner.originalTestURL;
  if (TestRunner.logEnabled)
    TestRunner.structuredLogger.info("SimpleTest START Loop " + TestRunner._currentLoop);

  TestRunner._urls = listURLs;
  $('testframe').src="";
  TestRunner._checkForHangs();
  TestRunner.runNextTest();
}

TestRunner.getNextUrl = function() {
    var url = "";
    
    if ((TestRunner._urls[TestRunner._currentTest] instanceof Object) && ('test' in TestRunner._urls[TestRunner._currentTest])) {
        url = TestRunner._urls[TestRunner._currentTest]['test']['url'];
        TestRunner.expected = TestRunner._urls[TestRunner._currentTest]['test']['expected'];
    } else {
        url = TestRunner._urls[TestRunner._currentTest];
        TestRunner.expected = 'pass';
    }
    return url;
}




TestRunner._haltTests = false;
TestRunner.runNextTest = function() {
    if (TestRunner._currentTest < TestRunner._urls.length &&
        !TestRunner._haltTests)
    {
        var url = TestRunner.getNextUrl();
        TestRunner.currentTestURL = url;

        $("current-test-path").innerHTML = url;

        TestRunner._currentTestStartTime = new Date().valueOf();
        TestRunner._timeoutFactor = 1;
        TestRunner._expectedMinAsserts = 0;
        TestRunner._expectedMaxAsserts = 0;

        TestRunner.structuredLogger.testStart(url);

        TestRunner._makeIframe(url, 0);
    } else {
        $("current-test").innerHTML = "<b>Finished</b>";
        
        if (TestRunner._urls.length > 1) {
            TestRunner._makeIframe("about:blank", 0);
        }

        var passCount = parseInt($("pass-count").innerHTML, 10);
        var failCount = parseInt($("fail-count").innerHTML, 10);
        var todoCount = parseInt($("todo-count").innerHTML, 10);

        if (passCount === 0 &&
            failCount === 0 &&
            todoCount === 0)
        {
            
            
            TestRunner.structuredLogger.testEnd('SimpleTest/TestRunner.js',
                                                "ERROR",
                                                "OK",
                                                "No checks actually run");
            
            $("fail-count").innerHTML = 1;
            
            var indicator = $("indicator");
            indicator.innerHTML = "Status: Fail (No checks actually run)";
            indicator.style.backgroundColor = "red";
        }

        SpecialPowers.unregisterProcessCrashObservers();

        TestRunner.structuredLogger.info("TEST-START | Shutdown");
        TestRunner.structuredLogger.info("Passed:  " + passCount);
        TestRunner.structuredLogger.info("Failed:  " + failCount);
        TestRunner.structuredLogger.info("Todo:    " + todoCount);
        TestRunner.structuredLogger.info("Slowest: " + TestRunner.slowestTestTime + 'ms - ' + TestRunner.slowestTestURL);

        
        if (TestRunner.repeat === 0) {
          TestRunner.structuredLogger.info("SimpleTest FINISHED");
        }

        if (TestRunner.repeat === 0 && TestRunner.onComplete) {
             TestRunner.onComplete();
         }

        if (TestRunner._currentLoop <= TestRunner.repeat && !TestRunner._haltTests) {
          TestRunner._currentLoop++;
          TestRunner.resetTests(TestRunner._urls);
          TestRunner._loopIsRestarting = true;
        } else {
          
          if (TestRunner.logEnabled) {
            TestRunner.structuredLogger.info("TEST-INFO | Ran " + TestRunner._currentLoop + " Loops");
            TestRunner.structuredLogger.info("SimpleTest FINISHED");
          }

          if (TestRunner.onComplete)
            TestRunner.onComplete();
       }
       TestRunner.generateFailureList();
    }
};

TestRunner.expectChildProcessCrash = function() {
    TestRunner._expectingProcessCrash = true;
};




TestRunner.testFinished = function(tests) {
    
    
    if (TestRunner._currentTest == TestRunner._lastTestFinished &&
        !TestRunner._loopIsRestarting) {
        TestRunner.structuredLogger.testEnd(TestRunner.currentTestURL,
                                            "ERROR",
                                            "OK",
                                            "called finish() multiple times");
        TestRunner.updateUI([{ result: false }]);
        return;
    }
    TestRunner._lastTestFinished = TestRunner._currentTest;
    TestRunner._loopIsRestarting = false;

    
    
    MemoryStats.dump(TestRunner._currentTest,
                     TestRunner.currentTestURL,
                     TestRunner.dumpOutputDirectory,
                     TestRunner.dumpAboutMemoryAfterTest,
                     TestRunner.dumpDMDAfterTest);

    function cleanUpCrashDumpFiles() {
        if (!SpecialPowers.removeExpectedCrashDumpFiles(TestRunner._expectingProcessCrash)) {
            TestRunner.structuredLogger.testEnd(TestRunner.currentTestURL,
                                                "ERROR",
                                                "OK",
                                                "This test did not leave any crash dumps behind, but we were expecting some!");
            tests.push({ result: false });
        }
        var unexpectedCrashDumpFiles =
            SpecialPowers.findUnexpectedCrashDumpFiles();
        TestRunner._expectingProcessCrash = false;
        if (unexpectedCrashDumpFiles.length) {
            TestRunner.structuredLogger.testEnd(TestRunner.currentTestURL,
                                                "ERROR",
                                                "OK",
                                                "This test left crash dumps behind, but we " +
                                                "weren't expecting it to!",
                                                {unexpected_crashdump_files: unexpectedCrashDumpFiles});
            tests.push({ result: false });
            unexpectedCrashDumpFiles.sort().forEach(function(aFilename) {
                TestRunner.structuredLogger.info("Found unexpected crash dump file " +
                                                 aFilename + ".");
            });
        }
    }

    function runNextTest() {
        if (TestRunner.currentTestURL != TestRunner.getLoadedTestURL()) {
            TestRunner.structuredLogger.testStatus(TestRunner.currentTestURL,
                                                   TestRunner.getLoadedTestURL(),
                                                   "FAIL",
                                                   "PASS",
                                                   "finished in a non-clean fashion, probably" +
                                                   " because it didn't call SimpleTest.finish()",
                                                   {loaded_test_url: TestRunner.getLoadedTestURL()});
            tests.push({ result: false });
        }

        var runtime = new Date().valueOf() - TestRunner._currentTestStartTime;

        TestRunner.structuredLogger.testEnd(TestRunner.currentTestURL,
                                            "OK",
                                            undefined,
                                            "Finished in " + runtime + "ms",
                                            {runtime: runtime}
        );

        if (TestRunner.slowestTestTime < runtime && TestRunner._timeoutFactor == 1) {
          TestRunner.slowestTestTime = runtime;
          TestRunner.slowestTestURL = TestRunner.currentTestURL;
        }

        TestRunner.updateUI(tests);

        
        if (TestRunner._urls.length == 1 && TestRunner.repeat <= 1) {
            TestRunner.testUnloaded();
            return;
        }

        var interstitialURL;
        if ($('testframe').contentWindow.location.protocol == "chrome:") {
            interstitialURL = "tests/SimpleTest/iframe-between-tests.html";
        } else {
            interstitialURL = "/tests/SimpleTest/iframe-between-tests.html";
        }
        
        $('testframe').contentWindow.addEventListener('unload', function() {
           var testwin = $('testframe').contentWindow;
           if (testwin.SimpleTest && testwin.SimpleTest._tests.length != testwin.SimpleTest.testsLength) {
             var wrongtestlength = testwin.SimpleTest._tests.length - testwin.SimpleTest.testsLength;
             var wrongtestname = '';
             for (var i = 0; i < wrongtestlength; i++) {
               wrongtestname = testwin.SimpleTest._tests[testwin.SimpleTest.testsLength + i].name;
               TestRunner.structuredLogger.testStatus(TestRunner.currentTestURL, wrongtestname, 'FAIL', 'PASS', "Result logged after SimpleTest.finish()");
             }
             TestRunner.updateUI([{ result: false }]);
           }
        } , false);
        TestRunner._makeIframe(interstitialURL, 0);
    }

    SpecialPowers.executeAfterFlushingMessageQueue(function() {
        cleanUpCrashDumpFiles();
        SpecialPowers.flushAllAppsLaunchable();
        SpecialPowers.flushPermissions(function () { SpecialPowers.flushPrefEnv(runNextTest); });
    });
};

TestRunner.testUnloaded = function() {
    
    
    
    if (SpecialPowers.isDebugBuild) {
        var newAssertionCount = SpecialPowers.assertionCount();
        var numAsserts = newAssertionCount - TestRunner._lastAssertionCount;
        TestRunner._lastAssertionCount = newAssertionCount;

        var url = TestRunner.getNextUrl();
        var max = TestRunner._expectedMaxAsserts;
        var min = TestRunner._expectedMinAsserts;
        if (numAsserts > max) {
            TestRunner.structuredLogger.testEnd(url,
                                                "ERROR",
                                                "OK",
                                                "Assertion count " + numAsserts + " is greater than expected range " +
                                                min + "-" + max + " assertions.",
                                                {assertions: numAsserts, min_asserts: min, max_asserts: max});
            TestRunner.updateUI([{ result: false }]);
        } else if (numAsserts < min) {
            TestRunner.structuredLogger.testEnd(url,
                                                "OK",
                                                "ERROR",
                                                "Assertion count " + numAsserts + " is less than expected range " +
                                                min + "-" + max + " assertions.",
                                                {assertions: numAsserts, min_asserts: min, max_asserts: max});
            TestRunner.updateUI([{ result: false }]);
        } else if (numAsserts > 0) {
            TestRunner.structuredLogger.testEnd(url,
                                                "ERROR",
                                                "ERROR",
                                                "Assertion count " + numAsserts + " within expected range " +
                                                min + "-" + max + " assertions.",
                                                {assertions: numAsserts, min_asserts: min, max_asserts: max});
        }
    }
    TestRunner._currentTest++;
    if (TestRunner.runSlower) {
        setTimeout(TestRunner.runNextTest, 1000);
    } else {
        TestRunner.runNextTest();
    }
};




TestRunner.countResults = function(tests) {
  var nOK = 0;
  var nNotOK = 0;
  var nTodo = 0;
  for (var i = 0; i < tests.length; ++i) {
    var test = tests[i];
    if (test.todo && !test.result) {
      nTodo++;
    } else if (test.result && !test.todo) {
      nOK++;
    } else {
      nNotOK++;
    }
  }
  return {"OK": nOK, "notOK": nNotOK, "todo": nTodo};
}




TestRunner.displayLoopErrors = function(tableName, tests) {
  if(TestRunner.countResults(tests).notOK >0){
    var table = $(tableName);
    var curtest;
    if (table.rows.length == 0) {
      
      var row = table.insertRow(table.rows.length);
      var cell = row.insertCell(0);
      var textNode = document.createTextNode("Test File Name:");
      cell.appendChild(textNode);
      cell = row.insertCell(1);
      textNode = document.createTextNode("Test:");
      cell.appendChild(textNode);
      cell = row.insertCell(2);
      textNode = document.createTextNode("Error message:");
      cell.appendChild(textNode);
    }

    
    for (var testnum in tests){
      curtest = tests[testnum];
      if( !((curtest.todo && !curtest.result) || (curtest.result && !curtest.todo)) ){
        
        row = table.insertRow(table.rows.length);
        cell = row.insertCell(0);
        textNode = document.createTextNode(TestRunner.currentTestURL);
        cell.appendChild(textNode);
        cell = row.insertCell(1);
        textNode = document.createTextNode(curtest.name);
        cell.appendChild(textNode);
        cell = row.insertCell(2);
        textNode = document.createTextNode((curtest.diag ? curtest.diag : "" ));
        cell.appendChild(textNode);
      }
    }
  }
}

TestRunner.updateUI = function(tests) {
  var results = TestRunner.countResults(tests);
  var passCount = parseInt($("pass-count").innerHTML) + results.OK;
  var failCount = parseInt($("fail-count").innerHTML) + results.notOK;
  var todoCount = parseInt($("todo-count").innerHTML) + results.todo;
  $("pass-count").innerHTML = passCount;
  $("fail-count").innerHTML = failCount;
  $("todo-count").innerHTML = todoCount;

  
  var indicator = $("indicator");
  if (failCount > 0) {
    indicator.innerHTML = "Status: Fail";
    indicator.style.backgroundColor = "red";
  } else if (passCount > 0) {
    indicator.innerHTML = "Status: Pass";
    indicator.style.backgroundColor = "#0d0";
  } else {
    indicator.innerHTML = "Status: ToDo";
    indicator.style.backgroundColor = "orange";
  }

  
  var trID = "tr-" + $('current-test-path').innerHTML;
  var row = $(trID);

  
  if (row != null) {
    var tds = row.getElementsByTagName("td");
    tds[0].style.backgroundColor = "#0d0";
    tds[0].innerHTML = parseInt(tds[0].innerHTML) + parseInt(results.OK);
    tds[1].style.backgroundColor = results.notOK > 0 ? "red" : "#0d0";
    tds[1].innerHTML = parseInt(tds[1].innerHTML) + parseInt(results.notOK);
    tds[2].style.backgroundColor = results.todo > 0 ? "orange" : "#0d0";
    tds[2].innerHTML = parseInt(tds[2].innerHTML) + parseInt(results.todo);
  }

  
  if (TestRunner.repeat > 0) {
    TestRunner.displayLoopErrors('fail-table', tests);
  }
}
