





function contentDispatchEvent(type, data, sync) {
  if (typeof(data) == "undefined") {
    data = {};
  }

  var element = document.createEvent("datacontainerevent");
  element.initEvent("contentEvent", true, false);
  element.setData("sync", sync);
  element.setData("type", type);
  element.setData("data", JSON.stringify(data));
  document.dispatchEvent(element);
}

function contentSyncEvent(type, data) {
  contentDispatchEvent(type, data, 1);
}

function contentAsyncEvent(type, data) {
  contentDispatchEvent(type, data, 0);
}









var TestRunner = {};
TestRunner.logEnabled = false;
TestRunner._currentTest = 0;
TestRunner.currentTestURL = "";
TestRunner._urls = [];

TestRunner.timeout = 5 * 60 * 1000; 
TestRunner.maxTimeouts = 4; 


if (typeof SpecialPowers != 'undefined') {
    TestRunner.ipcMode = SpecialPowers.hasContentProcesses();
} else {
    TestRunner.ipcMode = false;
}

TestRunner._expectingProcessCrash = false;




TestRunner._numTimeouts = 0;
TestRunner._currentTestStartTime = new Date().valueOf();
TestRunner._timeoutFactor = 1;

TestRunner._checkForHangs = function() {
  if (TestRunner._currentTest < TestRunner._urls.length) {
    var runtime = new Date().valueOf() - TestRunner._currentTestStartTime;
    if (runtime >= TestRunner.timeout * TestRunner._timeoutFactor) {
      var frameWindow = $('testframe').contentWindow.wrappedJSObject ||
                          $('testframe').contentWindow;
      frameWindow.SimpleTest.ok(false, "Test timed out.");

      
      
      if (++TestRunner._numTimeouts >= TestRunner.maxTimeouts) {
        TestRunner._haltTests = true;

        TestRunner.currentTestURL = "(SimpleTest/TestRunner.js)";
        frameWindow.SimpleTest.ok(false, TestRunner.maxTimeouts + " test timeouts, giving up.");
        var skippedTests = TestRunner._urls.length - TestRunner._currentTest;
        frameWindow.SimpleTest.ok(false, "Skipping " + skippedTests + " remaining tests.");
      }

      frameWindow.SimpleTest.finish();

      if (TestRunner._haltTests)
        return;
    }

    TestRunner.deferred = callLater(30, TestRunner._checkForHangs);
  }
}

TestRunner.requestLongerTimeout = function(factor) {
    TestRunner._timeoutFactor = factor;
}




TestRunner.onComplete = null;




TestRunner.logger = MochiKit.Logging.logger;

TestRunner.log = function(msg) {
    if (TestRunner.logEnabled) {
        TestRunner.logger.log(msg);
    } else {
        dump(msg + "\n");
    }
};

TestRunner.error = function(msg) {
    if (TestRunner.logEnabled) {
        TestRunner.logger.error(msg);
    } else {
        dump(msg + "\n");
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
        
        

        if (TestRunner.ipcMode) {
          contentAsyncEvent("Focus");
        }
        window.focus();
        iframe.focus();
        if (retry < 3) {
            window.setTimeout('TestRunner._makeIframe("'+url+'", '+(retry+1)+')', 1000);
            return;
        }

        TestRunner.log("Error: Unable to restore focus, expect failures and timeouts.");
    }
    window.scrollTo(0, $('indicator').offsetTop);
    iframe.src = url;
    iframe.name = url;
    iframe.width = "500";
    return iframe;
};







TestRunner.runTests = function () {
    TestRunner.log("SimpleTest START");

    if (typeof SpecialPowers != "undefined") {
        SpecialPowers.registerProcessCrashObservers();
    }

    TestRunner._urls = flattenArguments(arguments);
    $('testframe').src="";
    TestRunner._checkForHangs();
    window.focus();
    $('testframe').focus();
    TestRunner.runNextTest();
};




TestRunner._haltTests = false;
TestRunner.runNextTest = function() {
    if (TestRunner._currentTest < TestRunner._urls.length &&
        !TestRunner._haltTests)
    {
        var url = TestRunner._urls[TestRunner._currentTest];
        TestRunner.currentTestURL = url;

        $("current-test-path").innerHTML = url;

        TestRunner._currentTestStartTime = new Date().valueOf();
        TestRunner._timeoutFactor = 1;

        TestRunner.log("TEST-START | " + url); 

        TestRunner._makeIframe(url, 0);
    } else {
        $("current-test").innerHTML = "<b>Finished</b>";
        TestRunner._makeIframe("about:blank", 0);

        if (parseInt($("pass-count").innerHTML) == 0 &&
            parseInt($("fail-count").innerHTML) == 0 &&
            parseInt($("todo-count").innerHTML) == 0)
        {
          
          
          TestRunner.error("TEST-UNEXPECTED-FAIL | (SimpleTest/TestRunner.js) | No checks actually run.");
          
          $("fail-count").innerHTML = 1;
          
          var indicator = $("indicator");
          indicator.innerHTML = "Status: Fail (No checks actually run)";
          indicator.style.backgroundColor = "red";
        }

        TestRunner.log("TEST-START | Shutdown"); 
        TestRunner.log("Passed: " + $("pass-count").innerHTML);
        TestRunner.log("Failed: " + $("fail-count").innerHTML);
        TestRunner.log("Todo:   " + $("todo-count").innerHTML);
        TestRunner.log("SimpleTest FINISHED");

        if (TestRunner.onComplete) {
            TestRunner.onComplete();
        }
    }
};

TestRunner.expectChildProcessCrash = function() {
    if (typeof SpecialPowers == "undefined") {
        throw "TestRunner.expectChildProcessCrash must only be called from plain mochitests.";
    }

    TestRunner._expectingProcessCrash = true;
};




TestRunner.testFinished = function(tests) {
    function cleanUpCrashDumpFiles() {
        if (!SpecialPowers.removeExpectedCrashDumpFiles(TestRunner._expectingProcessCrash)) {
            TestRunner.error("TEST-UNEXPECTED-FAIL | " +
                             TestRunner.currentTestURL +
                             " | This test did not leave any crash dumps behind, but we were expecting some!");
            tests.push({ result: false });
        }
        var unexpectedCrashDumpFiles =
            SpecialPowers.findUnexpectedCrashDumpFiles();
        TestRunner._expectingProcessCrash = false;
        if (unexpectedCrashDumpFiles.length) {
            TestRunner.error("TEST-UNEXPECTED-FAIL | " +
                             TestRunner.currentTestURL +
                             " | This test left crash dumps behind, but we " +
                             "weren't expecting it to!");
            tests.push({ result: false });
            unexpectedCrashDumpFiles.sort().forEach(function(aFilename) {
                TestRunner.log("TEST-INFO | Found unexpected crash dump file " +
                               aFilename + ".");
            });
        }
    }

    function runNextTest() {
        var runtime = new Date().valueOf() - TestRunner._currentTestStartTime;
        TestRunner.log("TEST-END | " +
                       TestRunner._urls[TestRunner._currentTest] +
                       " | finished in " + runtime + "ms");

        TestRunner.updateUI(tests);
        TestRunner._currentTest++;
        TestRunner.runNextTest();
    }

    if (typeof SpecialPowers != 'undefined') {
        SpecialPowers.executeAfterFlushingMessageQueue(function() {
            cleanUpCrashDumpFiles();
            runNextTest();
        });
    } else {
        runNextTest();
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
  var tds = row.getElementsByTagName("td");
  tds[0].style.backgroundColor = "#0d0";
  tds[0].innerHTML = results.OK;
  tds[1].style.backgroundColor = results.notOK > 0 ? "red" : "#0d0";
  tds[1].innerHTML = results.notOK;
  tds[2].style.backgroundColor = results.todo > 0 ? "orange" : "#0d0";
  tds[2].innerHTML = results.todo;
}
