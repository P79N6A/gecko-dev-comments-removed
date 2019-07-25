
























function RequestTracker(id, timeLimit ) {
  this.id = id;
  this.timeLimit = timeLimit;

  if (arguments.length > 2) {
    this.mustReset  = true;
    this.resetAfter = arguments[2];
    this.resetTo    = arguments[3];
  }

  this.hasFired = false;
}
RequestTracker.prototype = {
  


  startXHR: function() {
    var req = new XMLHttpRequest();
    this.request = req;
    req.open("GET", "file_XHR_timeout.sjs");
    req.onerror   = this;
    req.onload    = this;
    req.onabort   = this;
    req.ontimeout = this;

    req.timeout = this.timeLimit;
    
    if (this.mustReset) {
      var resetTo = this.resetTo;
      window.setTimeout(function() {
        req.timeout = resetTo;
      }, this.resetAfter);
    }

    req.send(null);
  },

  




  getMessage: function() {
    var rv = this.id + ", ";
    if (this.mustReset) {
      rv += "original timeout at " + this.timeLimit + ", ";
      rv += "reset at " + this.resetAfter + " to " + this.resetTo;
    }
    else {
      rv += "timeout scheduled at " + this.timeLimit;
    }
    return rv;
  },

  




  handleEvent: function(evt) {
    if (this.hasFired) {
      ok(false, "Only one event should fire: " + this.getMessage());
      return;
    }
    this.hasFired = true;

    var type = evt.type, expectedType;
    
    var timeLimit = this.mustReset && (this.resetAfter < Math.min(3000, this.timeLimit)) ?
                    this.resetTo :
                    this.timeLimit;
    if ((timeLimit == 0) || (timeLimit >= 3000)) {
      expectedType = "load";
    }
    else {
      expectedType = "timeout";
    }
    is(type, expectedType, this.getMessage());
    TestCounter.testComplete();
  }
};







function AbortedRequest(shouldAbort, abortDelay) {
  this.shouldAbort = shouldAbort;
  this.abortDelay  = abortDelay;
  this.hasFired    = false;
}
AbortedRequest.prototype = {
  


  startXHR: function() {
    var req = new XMLHttpRequest();
    this.request = req;
    req.open("GET", "file_XHR_timeout.sjs");
    req.onerror   = this;
    req.onload    = this;
    req.onabort   = this;
    req.ontimeout = this;

    req.timeout = 2000;
    var _this = this;

    function abortReq() {
      req.abort();
    }

    if (!this.shouldAbort) {
      window.setTimeout(function() {
        try {
          _this.noEventsFired();
        }
        catch (e) {
          ok(false, "Unexpected error: " + e);
          TestCounter.testComplete();
        }
      }, 5000);
    }
    else {
      
      req.send();
      if (this.abortDelay == -1) {
        abortReq();
      }
      else {
        window.setTimeout(abortReq, this.abortDelay);
      }
    }
  },

  


  noEventsFired: function() {
    ok(!this.hasFired, "No events should fire for an unsent, unaborted request");
    
    TestCounter.testComplete();
  },

  




  getMessage: function() {
    return "time to abort is " + this.abortDelay + ", timeout set at 2000";
  },

  




  handleEvent: function(evt) {
    if (this.hasFired) {
      ok(false, "Only abort event should fire: " + this.getMessage());
      return;
    }
    this.hasFired = true;

    var expectedEvent = (this.abortDelay >= 2000) ? "timeout" : "abort";
    is(evt.type, expectedEvent, this.getMessage());
    TestCounter.testComplete();
  }
};

var SyncRequestSettingTimeoutAfterOpen = {
  startXHR: function() {
    var pass = false;
    var req = new XMLHttpRequest();
    req.open("GET", "file_XHR_timeout.sjs", false);
    try {
      req.timeout = 1000;
    }
    catch (e) {
      pass = true;
    }
    ok(pass, "Synchronous XHR must not allow a timeout to be set");
    TestCounter.testComplete();
  }
};

var SyncRequestSettingTimeoutBeforeOpen = {
  startXHR: function() {
    var pass = false;
    var req = new XMLHttpRequest();
    req.timeout = 1000;
    try {
      req.open("GET", "file_XHR_timeout.sjs", false);
    }
    catch (e) {
      pass = true;
    }
    ok(pass, "Synchronous XHR must not allow a timeout to be set");
    TestCounter.testComplete();
  }
};

var TestRequests = [
  
  new RequestTracker("no time out scheduled, load fires normally", 0),
  new RequestTracker("load fires normally", 5000),
  new RequestTracker("timeout hit before load", 2000),

  
  new RequestTracker("load fires normally with no timeout set, twice", 0, 2000, 0),
  new RequestTracker("load fires normally with same timeout set twice", 5000, 2000, 5000),
  new RequestTracker("timeout fires normally with same timeout set twice", 2000, 1000, 2000),

  new RequestTracker("timeout disabled after initially set", 5000, 2000, 0),
  new RequestTracker("timeout overrides load after a delay", 5000, 1000, 2000),
  new RequestTracker("timeout enabled after initially disabled", 0, 2000, 5000),

  new RequestTracker("timeout set to expiring value after load fires", 5000, 4000, 1000),
  new RequestTracker("timeout set to expired value before load fires", 5000, 2000, 1000),
  new RequestTracker("timeout set to non-expiring value after timeout fires", 1000, 2000, 5000),

  
  new AbortedRequest(false),
  new AbortedRequest(true, -1),
  new AbortedRequest(true, 0),
  new AbortedRequest(true, 1000),
  new AbortedRequest(true, 5000),

  
  SyncRequestSettingTimeoutAfterOpen,
  SyncRequestSettingTimeoutBeforeOpen
];


var TestCounter = {
  testComplete: function() {
    
    window.setTimeout(function() {
      TestCounter.next();
    }, 5000);
  },

  next: function() {
    var test = TestRequests.shift();

    if (test) {
      test.startXHR();
    }
    else {
      SimpleTest.finish();
    }
  }
};


(function() {
  SimpleTest.waitForExplicitFinish();
  SimpleTest.requestLongerTimeout(TestRequests.length);
  var msg = "This test will take approximately " + (TestRequests.length * 10)
  msg += " seconds to complete, at most.";
  document.getElementById("content").firstChild.nodeValue = msg;
  TestCounter.next();
})();
