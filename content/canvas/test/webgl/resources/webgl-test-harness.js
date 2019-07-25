


















































































WebGLTestHarnessModule = function() {




var log = function(msg) {
  if (window.console && window.console.log) {
    window.console.log(msg);
  }
};






var loadTextFileSynchronous = function(url) {
  var error = 'loadTextFileSynchronous failed to load url "' + url + '"';
  var request;
  if (window.XMLHttpRequest) {
    request = new XMLHttpRequest();
    if (request.overrideMimeType) {
      request.overrideMimeType('text/plain');
    }
  } else {
    throw 'XMLHttpRequest is disabled';
  }
  request.open('GET', url, false);
  request.send(null);
  if (request.readyState != 4) {
    throw error;
  }
  return request.responseText;
};

var getFileList = function(url) {
  var files = [];
  if (url.substr(url.length - 4) == '.txt') {
    var lines = loadTextFileSynchronous(url).split('\n');
    var prefix = '';
    var lastSlash = url.lastIndexOf('/');
    if (lastSlash >= 0) {
      prefix = url.substr(0, lastSlash + 1);
    }
    for (var ii = 0; ii < lines.length; ++ii) {
      var str = lines[ii].replace(/^\s\s*/, '').replace(/\s\s*$/, '');
      if (str.length > 4 &&
          str[0] != '#' &&
          str[0] != ";" &&
          str.substr(0, 2) != "//") {
        new_url = prefix + str;
        files = files.concat(getFileList(new_url));
      }
    }
  } else {
    files.push(url);
  }
  return files;
}

var TestFile = function(url) {
  this.url = url;
};

var TestHarness = function(iframe, filelistUrl, reportFunc) {
  this.window = window;
  this.iframe = iframe;
  this.reportFunc = reportFunc;
  try {
    var files = getFileList(filelistUrl);
  } catch (e) {
    this.reportFunc(
        TestHarness.reportType.FINISHED_ALL_TESTS,
        'Unable to load tests. Are you running locally?\n' +
        'You need to run from a server or configure your\n' +
        'browser to allow access to local files (not recommended).\n\n' +
        'Note: An easy way to run from a server:\n\n' +
        '\tcd path_to_tests\n' +
        '\tpython -m SimpleHTTPServer\n\n' +
        'then point your browser to ' +
          '<a href="http://localhost:8000/webgl-conformance-tests.html">' +
          'http://localhost:8000/webgl-conformance-tests.html</a>',
        false)
    return;
  }
  this.files = [];
  for (var ii = 0; ii < files.length; ++ii) {
    this.files.push(new TestFile(files[ii]));
    this.reportFunc(TestHarness.reportType.ADD_PAGE, files[ii], undefined);
  }
  this.nextFileIndex = files.length;
  this.lastFileIndex = files.length;
  this.timeoutDelay = 20000;
}

TestHarness.reportType = {
  ADD_PAGE: 1,
  START_PAGE: 2,
  TEST_RESULT: 3,
  FINISH_PAGE: 4,
  FINISHED_ALL_TESTS: 5
};

TestHarness.prototype.runTests = function(opt_start, opt_count) {
  var count = opt_count || this.files.length;
  this.nextFileIndex = opt_start || 0;
  this.lastFileIndex = this.nextFileIndex + count;
  this.startNextFile();
};

TestHarness.prototype.setTimeout = function() {
  var that = this;
  this.timeoutId = this.window.setTimeout(function() {
      that.timeout();
    }, this.timeoutDelay);
};

TestHarness.prototype.clearTimeout = function() {
  this.window.clearTimeout(this.timeoutId);
};

TestHarness.prototype.startNextFile = function() {
  if (this.nextFileIndex >= this.lastFileIndex) {
    log("done");
    this.reportFunc(TestHarness.reportType.FINISHED_ALL_TESTS,
                    '', true);
  } else {
    this.currentFile = this.files[this.nextFileIndex++];
    log("loading: " + this.currentFile.url);
    if (this.reportFunc(TestHarness.reportType.START_PAGE,
                        this.currentFile.url, undefined)) {
      this.iframe.src = this.currentFile.url;
      this.setTimeout();
    } else {
      this.reportResults(false, "skipped");
      this.notifyFinished();
    }
  }
};

TestHarness.prototype.reportResults = function (success, msg) {
  this.clearTimeout();
  log(success ? "PASS" : "FAIL", msg);
  this.reportFunc(TestHarness.reportType.TEST_RESULT, msg, success);
  
  this.setTimeout();
};

TestHarness.prototype.notifyFinished = function () {
  this.clearTimeout();
  var url = this.currentFile ? this.currentFile.url : 'unknown';
  log(url + ": finished");
  this.reportFunc(TestHarness.reportType.FINISH_PAGE, url, true);
  this.startNextFile();
};

TestHarness.prototype.timeout = function() {
  this.clearTimeout();
  var url = this.currentFile ? this.currentFile.url : 'unknown';
  log(url + ": timeout");
  this.reportFunc(TestHarness.reportType.FINISH_PAGE, url, undefined);
  this.startNextFile();
};

TestHarness.prototype.setTimeoutDelay = function(x) {
  this.timeoutDelay = x;
};

return {
    'TestHarness': TestHarness
  };

}();



