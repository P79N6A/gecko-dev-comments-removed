







var TestRunner = {};
TestRunner.logEnabled = false;
TestRunner._currentTest = 0;
TestRunner.currentTestURL = "";
TestRunner._urls = [];

TestRunner.timeout = 5 * 60 * 1000; 
TestRunner.maxTimeouts = 4; 




TestRunner._numTimeouts = 0;
TestRunner._currentTestStartTime = new Date().valueOf();

TestRunner._checkForHangs = function() {
  if (TestRunner._currentTest < TestRunner._urls.length) {
    var runtime = new Date().valueOf() - TestRunner._currentTestStartTime;
    if (runtime >= TestRunner.timeout) {
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




TestRunner.onComplete = null;




TestRunner.logger = MochiKit.Logging.logger;




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
        
        
        window.focus();
        iframe.focus();
        if (retry < 3) {
            window.setTimeout('TestRunner._makeIframe("'+url+'", '+(retry+1)+')', 1000);
            return;
        }

        if (TestRunner.logEnabled) {
            TestRunner.logger.log("Error: Unable to restore focus, expect failures and timeouts.");
        }
    }
    window.scrollTo(0, $('indicator').offsetTop);
    iframe.src = url;
    iframe.name = url;
    iframe.width = "500";
    return iframe;
};







TestRunner.runTests = function () {
    if (TestRunner.logEnabled)
        TestRunner.logger.log("SimpleTest START");

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

        if (TestRunner.logEnabled)
            TestRunner.logger.log("Running " + url + "...");

        TestRunner._makeIframe(url, 0);
    } else {
        $("current-test").innerHTML = "<b>Finished</b>";
        TestRunner._makeIframe("about:blank", 0);

        if (parseInt($("pass-count").innerHTML) == 0 &&
            parseInt($("fail-count").innerHTML) == 0 &&
            parseInt($("todo-count").innerHTML) == 0)
        {
          
          
          if (TestRunner.logEnabled)
            TestRunner.logger.error("TEST-UNEXPECTED-FAIL | (SimpleTest/TestRunner.js) | No checks actually run.");
          
          $("fail-count").innerHTML = 1;
          
          var indicator = $("indicator");
          indicator.innerHTML = "Status: Fail (No checks actually run)";
          indicator.style.backgroundColor = "red";
        }

        if (TestRunner.logEnabled) {
            TestRunner.logger.log("Passed: " + $("pass-count").innerHTML);
            TestRunner.logger.log("Failed: " + $("fail-count").innerHTML);
            TestRunner.logger.log("Todo:   " + $("todo-count").innerHTML);
            TestRunner.logger.log("SimpleTest FINISHED");
        }

        if (TestRunner.onComplete)
            TestRunner.onComplete();
    }
};




TestRunner.testFinished = function(doc) {
    if (TestRunner.logEnabled)
        TestRunner.logger.debug("SimpleTest finished " +
                                TestRunner._urls[TestRunner._currentTest]);

    TestRunner.updateUI();
    TestRunner._currentTest++;
    TestRunner.runNextTest();
};




TestRunner.countResults = function(doc) {
  var nOK = withDocument(doc,
     partial(getElementsByTagAndClassName, 'div', 'test_ok')
  ).length;
  var nNotOK = withDocument(doc,
     partial(getElementsByTagAndClassName, 'div', 'test_not_ok')
  ).length;
  var nTodo = withDocument(doc,
     partial(getElementsByTagAndClassName, 'div', 'test_todo')
  ).length;
  return {"OK": nOK, "notOK": nNotOK, "todo": nTodo};
}

TestRunner.updateUI = function() {
  var testFrame = $('testframe');
  var results = TestRunner.countResults(testFrame.contentDocument ||
                                        testFrame.contentWindow.document);
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
