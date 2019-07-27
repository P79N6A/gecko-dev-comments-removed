


















































































WebGLTestHarnessModule = function() {




var log = function(msg) {
  if (window.console && window.console.log) {
    window.console.log(msg);
  }
};







var loadTextFileAsynchronous = function(url, callback) {
  log ("loading: " + url);
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
  try {
    request.open('GET', url, true);
    request.onreadystatechange = function() {
      if (request.readyState == 4) {
        var text = '';
        
        
        
        
        var success = request.status == 200 || request.status == 0;
        if (success) {
          text = request.responseText;
        }
        log("loaded: " + url);
        callback(success, text);
      }
    };
    request.send(null);
  } catch (e) {
    log("failed to load: " + url);
    callback(false, '');
  }
};




var greaterThanOrEqualToVersion = function(have, want) {
  have = have.split(" ")[0].split(".");
  want = want.split(" ")[0].split(".");

  
  
  
  

  for (var ii = 0; ii < want.length; ++ii) {
    var wantNum = parseInt(want[ii]);
    var haveNum = have[ii] ? parseInt(have[ii]) : 0
    if (haveNum < wantNum) {
      return false;
    }
  }
  return true;
};
























var getFileList = function(url, callback, options) {
  var files = [];

  var copyObject = function(obj) {
    return JSON.parse(JSON.stringify(obj));
  };

  var toCamelCase = function(str) {
    return str.replace(/-([a-z])/g, function (g) { return g[1].toUpperCase() });
  };

  var globalOptions = copyObject(options);
  globalOptions.defaultVersion = "1.0";

  var getFileListImpl = function(prefix, line, hierarchicalOptions, callback) {
    var files = [];

    var args = line.split(/\s+/);
    var nonOptions = [];
    var useTest = true;
    var testOptions = {};
    for (var jj = 0; jj < args.length; ++jj) {
      var arg = args[jj];
      if (arg[0] == '-') {
        if (arg[1] != '-') {
          throw ("bad option at in " + url + ":" + (ii + 1) + ": " + str);
        }
        var option = arg.substring(2);
        switch (option) {
          case 'min-version':
            ++jj;
            testOptions[toCamelCase(option)] = args[jj];
            break;
          default:
            throw ("bad unknown option '" + option + "' at in " + url + ":" + (ii + 1) + ": " + str);
        }
      } else {
        nonOptions.push(arg);
      }
    }
    var url = prefix + nonOptions.join(" ");

    if (url.substr(url.length - 4) != '.txt') {
      var minVersion = testOptions.minVersion;
      if (!minVersion) {
        minVersion = hierarchicalOptions.defaultVersion;
      }

      if (globalOptions.minVersion) {
        useTest = greaterThanOrEqualToVersion(minVersion, globalOptions.minVersion);
      } else {
        useTest = greaterThanOrEqualToVersion(globalOptions.version, minVersion);
      }
    }

    if (!useTest) {
      callback(true, []);
      return;
    }

    if (url.substr(url.length - 4) == '.txt') {
      
      if (testOptions.minVersion) {
        hierarchicalOptions.defaultVersion = testOptions.minVersion;
      }
      loadTextFileAsynchronous(url, function() {
        return function(success, text) {
          if (!success) {
            callback(false, '');
            return;
          }
          var lines = text.split('\n');
          var prefix = '';
          var lastSlash = url.lastIndexOf('/');
          if (lastSlash >= 0) {
            prefix = url.substr(0, lastSlash + 1);
          }
          var fail = false;
          var count = 1;
          var index = 0;
          for (var ii = 0; ii < lines.length; ++ii) {
            var str = lines[ii].replace(/^\s\s*/, '').replace(/\s\s*$/, '');
            if (str.length > 4 &&
                str[0] != '#' &&
                str[0] != ";" &&
                str.substr(0, 2) != "//") {
              ++count;
              getFileListImpl(prefix, str, copyObject(hierarchicalOptions), function(index) {
                return function(success, new_files) {
                  log("got files: " + new_files.length);
                  if (success) {
                    files[index] = new_files;
                  }
                  finish(success);
                };
              }(index++));
            }
          }
          finish(true);

          function finish(success) {
            if (!success) {
              fail = true;
            }
            --count;
            log("count: " + count);
            if (!count) {
              callback(!fail, files);
            }
          }
        }
      }());
    } else {
      files.push(url);
      callback(true, files);
    }
  };

  getFileListImpl('', url, globalOptions, function(success, files) {
    
    var flat = [];
    flatten(files);
    function flatten(files) {
      for (var ii = 0; ii < files.length; ++ii) {
        var value = files[ii];
        if (typeof(value) == "string") {
          flat.push(value);
        } else {
          flatten(value);
        }
      }
    }
    callback(success, flat);
  });
};

var TestFile = function(url) {
  this.url = url;
};

var TestHarness = function(iframe, filelistUrl, reportFunc, options) {
  this.window = window;
  this.iframe = iframe;
  this.reportFunc = reportFunc;
  this.timeoutDelay = 20000;
  this.files = [];

  var that = this;
  getFileList(filelistUrl, function() {
    return function(success, files) {
      that.addFiles_(success, files);
    };
  }(), options);

};

TestHarness.reportType = {
  ADD_PAGE: 1,
  READY: 2,
  START_PAGE: 3,
  TEST_RESULT: 4,
  FINISH_PAGE: 5,
  FINISHED_ALL_TESTS: 6
};

TestHarness.prototype.addFiles_ = function(success, files) {
  if (!success) {
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
  log("total files: " + files.length);
  for (var ii = 0; ii < files.length; ++ii) {
    log("" + ii + ": " + files[ii]);
    this.files.push(new TestFile(files[ii]));
    this.reportFunc(TestHarness.reportType.ADD_PAGE, files[ii], undefined);
  }
  this.reportFunc(TestHarness.reportType.READY, undefined, undefined);
}

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



