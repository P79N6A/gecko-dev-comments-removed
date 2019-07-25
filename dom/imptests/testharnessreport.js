



var W3CTest = {
  






  "expectedFailures": {},

  


  "tests": [],

  


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
    if (this.runner.currentTestURL)
      msg += this.runner.currentTestURL;
    msg += " | " + test.message;
    this.runner[(test.result === !test.todo) ? "log" : "error"](msg);
  },

  







  "report": function(test) {
    this.tests.push(test);
    this._log(test);
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
    this.runner.testFinished(this.tests);
  }
};
(function() {
  if (!W3CTest.runner) {
    return;
  }
  
  
  var request = new XMLHttpRequest();
  request.open("GET", "/tests/dom/imptests/failures/" + W3CTest.getURL() + ".json", false);
  request.send();
  if (request.status === 200) {
    W3CTest.expectedFailures = JSON.parse(request.responseText);
  } else if (request.status !== 404) {
    is(request.status, 404, "Request status neither 200 nor 404");
  }

  add_result_callback(W3CTest.result.bind(W3CTest));
  add_completion_callback(W3CTest.finish.bind(W3CTest));
  setup({
    "output": false,
    "timeout": 1000000,
  });
})();
