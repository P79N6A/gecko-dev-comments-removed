



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

  


  "pathprefix": "/tests/dom/imptests/",

  



  "getPath": function() {
    var url = this.getURL();
    if (!url.startsWith(this.pathprefix)) {
      return "";
    }
    return url.substring(this.pathprefix.length);
  },

  


  "getURL": function() {
    return this.runner ? this.runner.currentTestURL : location.pathname;
  },

  


  "reportResults": function() {
    var element = function element(aLocalName) {
      var xhtmlNS = "http://www.w3.org/1999/xhtml";
      return document.createElementNS(xhtmlNS, aLocalName);
    };

    var stylesheet = element("link");
    stylesheet.setAttribute("rel", "stylesheet");
    stylesheet.setAttribute("href", "/resources/testharness.css");
    var heads = document.getElementsByTagName("head");
    if (heads.length) {
      heads[0].appendChild(stylesheet);
    }

    var log = document.getElementById("log");
    if (!log) {
      return;
    }
    var section = log.appendChild(element("section"));
    section.id = "summary";
    section.appendChild(element("h2")).textContent = "Details";

    var table = section.appendChild(element("table"));
    table.id = "results";

    var tr = table.appendChild(element("thead")).appendChild(element("tr"));
    for (var header of ["Result", "Test Name", "Message"]) {
      tr.appendChild(element("th")).textContent = header;
    }
    var statuses = [
      ["Unexpected Fail", "Pass"],
      ["Known Fail", "Unexpected Pass"]
    ];
    var tbody = table.appendChild(element("tbody"));
    for (var test of this.tests) {
      tr = tbody.appendChild(element("tr"));
      tr.className = (test.result === !test.todo ? "pass" : "fail");
      tr.appendChild(element("td")).textContent =
        statuses[+test.todo][+test.result];
      tr.appendChild(element("td")).textContent = test.name;
      tr.appendChild(element("td")).textContent = test.message;
    }
  },

  



  "formatTestMessage": function(aTest) {
    return aTest.name + (aTest.message ? ": " + aTest.message : "");
  },

  


  "_log": function(test) {
    var url = this.getURL();
    var msg = this.prefixes[+test.todo][+test.result] + " | ";
    if (url) {
      msg += url;
    }
    msg += " | " + this.formatTestMessage(test);
    if (this.runner) {
      this.runner[(test.result === !test.todo) ? "log" : "error"](msg);
    } else {
      dump(msg + "\n");
    }
  },

  


  "_logCollapsedMessages": function() {
    if (this.collapsedMessages) {
      this._log({
        "name": document.title,
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
    var url = this.getPath();
    this.report({
      "name": test.name,
      "message": test.message || "",
      "result": test.status === test.PASS,
      "todo": this._todo(test)
    });
    if (this.dumpFailures && test.status !== test.PASS) {
      this.failures[test.name] = true;
    }
  },

  



  "finish": function(tests, status) {
    var url = this.getPath();
    this.report({
      "name": "Finished test",
      "message": "Status: " + status.status,
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
    if (this.runner) {
      this.runner.testFinished(this.tests.map(function(aTest) {
        return {
          "message": this.formatTestMessage(aTest),
          "result": aTest.result,
          "todo": aTest.todo
        };
      }, this));
    } else {
      this.reportResults();
    }
  },

  



  "logFailure": function(aTestName, aMessage) {
    this.report({
      "name": aTestName,
      "message": aMessage,
      "result": false,
      "todo": false
    });
  },

  



  "timeout": function() {
    this.logFailure("Timeout", "Test runner timed us out.");
    timeout();
  }
};
(function() {
  try {
    var path = W3CTest.getPath();
    if (path) {
      
      
      var url = W3CTest.pathprefix + "failures/" + path + ".json";
      var request = new XMLHttpRequest();
      request.open("GET", url, false);
      request.send();
      if (request.status === 200) {
        W3CTest.expectedFailures = JSON.parse(request.responseText);
      } else if (request.status !== 404) {
        W3CTest.logFailure("Fetching failures file", "Request status was " + request.status);
      }
    }

    add_result_callback(W3CTest.result.bind(W3CTest));
    add_completion_callback(W3CTest.finish.bind(W3CTest));
    setup({
      "output": false,
      "explicit_timeout": true
    });
    
    window.onerror = function(message, fileName, lineNumber) {
      W3CTest.logFailure("Uncaught exception", fileName + " (" + lineNumber + "): " + message);
    };
  } catch (e) {
    W3CTest.logFailure("Harness setup", "Unexpected exception: " + e);
  }
})();
