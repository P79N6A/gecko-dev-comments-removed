





function getElement(id) {
    return ((typeof(id) == "string") ?
        document.getElementById(id) : id);
}

this.$ = this.getElement;

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









var TestRunner = {};
TestRunner.logEnabled = false;
TestRunner._currentTest = 0;
TestRunner._lastTestFinished = -1;
TestRunner.currentTestURL = "";
TestRunner.originalTestURL = "";
TestRunner._urls = [];
TestRunner._lastAssertionCount = 0;
TestRunner._expectedMinAsserts = 0;
TestRunner._expectedMaxAsserts = 0;

TestRunner.timeout = 5 * 60 * 1000; 
TestRunner.maxTimeouts = 4; 
TestRunner.runSlower = false;

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
TestRunner._currentLoop = 0;

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




TestRunner.logger = LogController;

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
        
        

        contentAsyncEvent("Focus");
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






TestRunner.getLoadedTestURL = function () {
    var prefix = "";
    
    if ($('testframe').contentWindow.location.protocol == "chrome:") {
      prefix = "chrome://mochitests";
    }
    return prefix + $('testframe').contentWindow.location.pathname;
};







TestRunner.runTests = function () {
    TestRunner.log("SimpleTest START");
    TestRunner.originalTestURL = $("current-test").innerHTML;

    SpecialPowers.registerProcessCrashObservers();

    TestRunner._urls = flattenArguments(arguments);
    $('testframe').src="";
    TestRunner._checkForHangs();
    window.focus();
    $('testframe').focus();
    TestRunner.runNextTest();
};





TestRunner.resetTests = function(listURLs) {
  TestRunner._currentTest = 0;
  
  $("current-test").innerHTML = TestRunner.originalTestURL;
  if (TestRunner.logEnabled)
    TestRunner.log("SimpleTest START Loop " + TestRunner._currentLoop);

  TestRunner._urls = listURLs;
  $('testframe').src="";
  TestRunner._checkForHangs();
  window.focus();
  $('testframe').focus();
  TestRunner.runNextTest();
}




TestRunner.loopTest = function(testPath) {
  
  document.getElementById("current-test-path").innerHTML = testPath;
  var numLoops = TestRunner.repeat;
  var completed = 0; 

  
  function checkComplete() {
    var testWindow = window.open(testPath, 'test window'); 
    if (testWindow.document.readyState == "complete") {
      
      TestRunner.currentTestURL = testPath;
      TestRunner.updateUI(testWindow.SimpleTest._tests);
      testWindow.close();
      if (TestRunner.repeat == completed  && TestRunner.onComplete) {
        TestRunner.onComplete();
      }
      completed++;
    }
    else {
      
      setTimeout(checkComplete, 1000);
    }
  }
  while (numLoops >= 0) {
    checkComplete();
    numLoops--;
  }
}




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
        TestRunner._expectedMinAsserts = 0;
        TestRunner._expectedMaxAsserts = 0;

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

        SpecialPowers.unregisterProcessCrashObservers();

        TestRunner.log("TEST-START | Shutdown"); 
        TestRunner.log("Passed: " + $("pass-count").innerHTML);
        TestRunner.log("Failed: " + $("fail-count").innerHTML);
        TestRunner.log("Todo:   " + $("todo-count").innerHTML);
        
        if (TestRunner.repeat == 0) {
          TestRunner.log("SimpleTest FINISHED");
        }

        if (TestRunner.repeat == 0 && TestRunner.onComplete) {
             TestRunner.onComplete();
         }

        if (TestRunner._currentLoop < TestRunner.repeat) {
          TestRunner._currentLoop++;
          TestRunner.resetTests(TestRunner._urls);
        } else {
          
          if (TestRunner.logEnabled) {
            TestRunner.log("TEST-INFO | Ran " + TestRunner._currentLoop + " Loops");
            TestRunner.log("SimpleTest FINISHED");
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
    
    
    if (TestRunner._currentTest == TestRunner._lastTestFinished) {
        TestRunner.error("TEST-UNEXPECTED-FAIL | " +
                         TestRunner.currentTestURL +
                         " | called finish() multiple times");
        return;
    }
    TestRunner._lastTestFinished = TestRunner._currentTest;

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
        if (TestRunner.currentTestURL != TestRunner.getLoadedTestURL()) {
            TestRunner.error("TEST-UNEXPECTED-FAIL | " +
                             TestRunner.currentTestURL +
                             " | " + TestRunner.getLoadedTestURL() +
                             " finished in a non-clean fashion, probably" +
                             " because it didn't call SimpleTest.finish()");
            tests.push({ result: false });
        }

        var runtime = new Date().valueOf() - TestRunner._currentTestStartTime;
        TestRunner.log("TEST-END | " +
                       TestRunner.currentTestURL +
                       " | finished in " + runtime + "ms");

        TestRunner.updateUI(tests);

        var interstitialURL;
        if ($('testframe').contentWindow.location.protocol == "chrome:") {
            interstitialURL = "tests/SimpleTest/iframe-between-tests.html";
        } else {
            interstitialURL = "/tests/SimpleTest/iframe-between-tests.html";
        }
        TestRunner._makeIframe(interstitialURL, 0);
    }

    SpecialPowers.executeAfterFlushingMessageQueue(function() {
        cleanUpCrashDumpFiles();
        SpecialPowers.flushPrefEnv(runNextTest);
    });
};

TestRunner.testUnloaded = function() {
    if (SpecialPowers.isDebugBuild) {
        var newAssertionCount = SpecialPowers.assertionCount();
        var numAsserts = newAssertionCount - TestRunner._lastAssertionCount;
        TestRunner._lastAssertionCount = newAssertionCount;

        var url = TestRunner._urls[TestRunner._currentTest];
        var max = TestRunner._expectedMaxAsserts;
        var min = TestRunner._expectedMinAsserts;
        if (numAsserts > max) {
            
            
            TestRunner.log("TEST-DETCEPXENU-FAIL | " + url + " | Assertion count " + numAsserts + " is greater than expected range " + min + "-" + max + " assertions.");
        } else if (numAsserts < min) {
            
            
            TestRunner.log("TEST-DETCEPXENU-PASS | " + url + " | Assertion count " + numAsserts + " is less than expected range " + min + "-" + max + " assertions.");
        } else if (numAsserts > 0) {
            TestRunner.log("TEST-KNOWN-FAIL | " + url + " | Assertion count " + numAsserts + " within expected range " + min + "-" + max + " assertions.");
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
