












var TIME_NORMAL_LOAD = 1000;
var TIME_LATE_TIMEOUT = 800;
var TIME_XHR_LOAD = 600;
var TIME_REGULAR_TIMEOUT = 400;
var TIME_SYNC_TIMEOUT = 200;
var TIME_DELAY = 200;




var STALLED_REQUEST_URL = "delay.py?ms=" + (TIME_XHR_LOAD);

var inWorker = false;
try {
  inWorker = !(self instanceof Window);
} catch (e) {
  inWorker = true;
}

if (!inWorker)
  STALLED_REQUEST_URL = "resources/" + STALLED_REQUEST_URL;

function message(obj) {
  if (inWorker)
    self.postMessage(obj);
  else
    self.postMessage(obj, "*");
}

function is(got, expected, msg) {
  var obj = {};
  obj.type = "is";
  obj.got = got;
  obj.expected = expected;
  obj.msg = msg;

  message(obj);
}

function ok(bool, msg) {
  var obj = {};
  obj.type = "ok";
  obj.bool = bool;
  obj.msg = msg;

  message(obj);
}



















function RequestTracker(async, id, timeLimit ) {
  this.async = async;
  this.id = id;
  this.timeLimit = timeLimit;

  if (arguments.length > 3) {
    this.mustReset  = true;
    this.resetAfter = arguments[3];
    this.resetTo    = arguments[4];
  }

  this.hasFired = false;
}
RequestTracker.prototype = {
  


  startXHR: function() {
    var req = new XMLHttpRequest();
    this.request = req;
    req.open("GET", STALLED_REQUEST_URL, this.async);
    var me = this;
    function handleEvent(e) { return me.handleEvent(e); };
    req.onerror = handleEvent;
    req.onload = handleEvent;
    req.onabort = handleEvent;
    req.ontimeout = handleEvent;

    req.timeout = this.timeLimit;

    if (this.mustReset) {
      var resetTo = this.resetTo;
      self.setTimeout(function() {
        req.timeout = resetTo;
      }, this.resetAfter);
    }

    try {
      req.send(null);
    }
    catch (e) {
      
      ok(!this.async && this.timeLimit < TIME_XHR_LOAD && e.name == "TimeoutError", "Unexpected error: " + e);
      TestCounter.testComplete();
    }
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
    
    var timeLimit = this.mustReset && (this.resetAfter < Math.min(TIME_XHR_LOAD, this.timeLimit)) ?
                    this.resetTo :
                    this.timeLimit;
    if ((timeLimit == 0) || (timeLimit >= TIME_XHR_LOAD)) {
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
    req.open("GET", STALLED_REQUEST_URL);
    var _this = this;
    function handleEvent(e) { return _this.handleEvent(e); };
    req.onerror = handleEvent;
    req.onload = handleEvent;
    req.onabort = handleEvent;
    req.ontimeout = handleEvent;

    req.timeout = TIME_REGULAR_TIMEOUT;

    function abortReq() {
      req.abort();
    }

    if (!this.shouldAbort) {
      self.setTimeout(function() {
        try {
          _this.noEventsFired();
        }
        catch (e) {
          ok(false, "Unexpected error: " + e);
          TestCounter.testComplete();
        }
      }, TIME_NORMAL_LOAD);
    }
    else {
      
      req.send();
      if (this.abortDelay == -1) {
        abortReq();
      }
      else {
        self.setTimeout(abortReq, this.abortDelay);
      }
    }
  },

  


  noEventsFired: function() {
    ok(!this.hasFired, "No events should fire for an unsent, unaborted request");
    
    TestCounter.testComplete();
  },

  




  getMessage: function() {
    return "time to abort is " + this.abortDelay + ", timeout set at " + TIME_REGULAR_TIMEOUT;
  },

  








  handleEvent: function(evt) {
    if (this.hasFired && evt.type != "abort") {
      ok(false, "Only abort event should fire: " + this.getMessage());
      return;
    }

    var expectedEvent = (this.abortDelay >= TIME_REGULAR_TIMEOUT && !this.hasFired) ? "timeout" : "abort";
    this.hasFired = true;
    is(evt.type, expectedEvent, this.getMessage());
    TestCounter.testComplete();
  }
};

var SyncRequestSettingTimeoutAfterOpen = {
  startXHR: function() {
    var pass = false;
    var req = new XMLHttpRequest();
    req.open("GET", STALLED_REQUEST_URL, false);
    try {
      req.timeout = TIME_SYNC_TIMEOUT;
    }
    catch (e) {
      pass = true;
    }
    ok(pass, "Synchronous XHR must not allow a timeout to be set - setting timeout must throw");
    TestCounter.testComplete();
  }
};

var SyncRequestSettingTimeoutBeforeOpen = {
  startXHR: function() {
    var pass = false;
    var req = new XMLHttpRequest();
    req.timeout = TIME_SYNC_TIMEOUT;
    try {
      req.open("GET", STALLED_REQUEST_URL, false);
    }
    catch (e) {
      pass = true;
    }
    ok(pass, "Synchronous XHR must not allow a timeout to be set - calling open() after timeout is set must throw");
    TestCounter.testComplete();
  }
};

var TestRequests = [];


var TestCounter = {
  testComplete: function() {
    
    self.setTimeout(function() {
      TestCounter.next();
    }, TIME_NORMAL_LOAD);
  },

  next: function() {
    var test = TestRequests.shift();

    if (test) {
      test.startXHR();
    }
    else {
      message("done");
    }
  }
};

function runTestRequests(testRequests) {
    TestRequests = testRequests;
    TestCounter.next();
}
