





function getElement(id) {
    return ((typeof(id) == "string") ?
        document.getElementById(id) : id); 
};   

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

function contentSyncEvent(type, data) {
  contentDispatchEvent(type, data, 1);
}

function contentAsyncEvent(type, data) {
  contentDispatchEvent(type, data, 0);
}


function extend(obj, skip) {        
    
    
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
};

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
};









var TestRunner = {};
TestRunner.logEnabled = false;
TestRunner._currentTest = 0;
TestRunner.currentTestURL = "";
TestRunner.originalTestURL = "";
TestRunner._urls = [];

TestRunner.timeout = 5 * 60 * 1000; 
TestRunner.maxTimeouts = 4; 

TestRunner.ipcMode = SpecialPowers.hasContentProcesses();
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

    setTimeout(TestRunner._checkForHangs, 30000);
  }
}

TestRunner.requestLongerTimeout = function(factor) {
    TestRunner._timeoutFactor = factor;
}




TestRunner.repeat = 0;
TestRunner._currentLoop = 0;




TestRunner.onComplete = null;




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
    }
};

TestRunner.expectChildProcessCrash = function() {
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

    SpecialPowers.executeAfterFlushingMessageQueue(function() {
        cleanUpCrashDumpFiles();
        runNextTest();
    });
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
  var tds = row.getElementsByTagName("td");
  tds[0].style.backgroundColor = "#0d0";
  tds[0].innerHTML = parseInt(tds[0].innerHTML) + parseInt(results.OK);
  tds[1].style.backgroundColor = results.notOK > 0 ? "red" : "#0d0";
  tds[1].innerHTML = parseInt(tds[1].innerHTML) + parseInt(results.notOK);
  tds[2].style.backgroundColor = results.todo > 0 ? "orange" : "#0d0";
  tds[2].innerHTML = parseInt(tds[2].innerHTML) + parseInt(results.todo);

  
  if (TestRunner.repeat > 0) {
    TestRunner.displayLoopErrors('fail-table', tests);
  }
}
