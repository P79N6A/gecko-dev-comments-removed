



var W3CTest = {
  






  "expectedFailures": {},

  


  "dumpFailures": false,

  



  "failures": {},

  


  "tests": [],

  



  "collapsedMessages": 0,
  "MAX_COLLAPSED_MESSAGES": 100,

  


  "runner": parent === this ? null : parent.TestRunner || parent.wrappedJSObject.TestRunner,

  



  "prefixes": [
    ["TEST-UNEXPECTED-FAIL", "TEST-PASS"],
    ["TEST-KNOWN-FAIL", "TEST-UNEXPECTED-PASS"]
  ],

  



  "getURL": function() {
    return this.runner.currentTestURL.substring("/tests/dom/imptests/".length);
  },

  


  "_log": function(test) {
    var msg = this.prefixes[+test.todo][+test.result] + " | ";
    if (this.runner.currentTestURL) {
      msg += this.runner.currentTestURL;
    }
    msg += " | " + test.message;
    this.runner[(test.result === !test.todo) ? "log" : "error"](msg);
  },

  "_logCollapsedMessages": function() {
    if (this.collapsedMessages) {
      this._log({
        "result": true,
        "todo": false,
        "message": "Elided " + this.collapsedMessages + " passes or known failures."
      });
    }
    this.collapsedMessages = 0;
  },

  



  "_maybeLog": function(test) {
    var success = (test.result === !test.todo);
    if (success && ++this.collapsedMessages < this.MAX_COLLAPSED_MESSAGES) {
      return;
    }
    this._logCollapsedMessages();
    this._log(test);
  },

  







  "report": function(test) {
    this.tests.push(test);
    this._maybeLog(test);
  },

  


  "_todo": function(test) {
    if (this.expectedFailures === "all") {
      return true;
    }
    var value = this.expectedFailures[test.name];
    return value === true || (value === "debug" && !!SpecialPowers.isDebugBuild);
  },

  



  "result": function(test) {
    var url = this.getURL();
    this.report({
      "message": test.name + (test.message ? "; " + test.message : ""),
      "result": test.status === test.PASS,
      "todo": this._todo(test)
    });
    if (this.dumpFailures && test.status !== test.PASS) {
      this.failures[test.name] = true;
    }
  },

  



  "finish": function(tests, status) {
    var url = this.getURL();
    this.report({
      "message": "Finished test, status " + status.status,
      "result": status.status === status.OK,
      "todo":
        url in this.expectedFailures &&
        this.expectedFailures[url] === "error"
    });

    this._logCollapsedMessages();

    if (this.dumpFailures) {
      dump("@@@ @@@ Failures\n");
      dump(url + "@@@" + JSON.stringify(this.failures) + "\n");
    }
    this.runner.testFinished(this.tests);
  },

  



  "logFailure": function(message) {
    this.report({
      "message": message,
      "result": false,
      "todo": false
    });
  },

  



  "timeout": function() {
    this.logFailure("Test runner timed us out.");
    timeout();
  }
};
(function() {
  try {
    if (!W3CTest.runner) {
      return;
    }
    
    
    var request = new XMLHttpRequest();
    request.open("GET", "/tests/dom/imptests/failures/" + W3CTest.getURL() + ".json", false);
    request.send();
    if (request.status === 200) {
      W3CTest.expectedFailures = JSON.parse(request.responseText);
    } else if (request.status !== 404) {
      W3CTest.logFailure("Request status was " + request.status);
    }

    add_result_callback(W3CTest.result.bind(W3CTest));
    add_completion_callback(W3CTest.finish.bind(W3CTest));
    setup({
      "output": false,
      "explicit_timeout": true
    });
  } catch (e) {
    W3CTest.logFailure("Unexpected exception: " + e);
  }
})();
