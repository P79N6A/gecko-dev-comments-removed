version(180);

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;


let ch = Cc["@mozilla.org/security/hash;1"].
         createInstance(Ci.nsICryptoHash);

let ds = Cc["@mozilla.org/file/directory_service;1"]
  .getService(Ci.nsIProperties);

let provider = {
  getFile: function(prop, persistent) {
    persistent.value = true;
    if (prop == "ExtPrefDL")
      return [ds.get("CurProcD", Ci.nsIFile)];
    else if (prop == "ProfD")
      return ds.get("CurProcD", Ci.nsIFile);
    throw Cr.NS_ERROR_FAILURE;
  },
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIDirectoryServiceProvider) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};
ds.QueryInterface(Ci.nsIDirectoryService).registerProvider(provider);

do_bind_resource(do_get_file("modules"), "weave");

function loadInSandbox(aUri) {
  var sandbox = Components.utils.Sandbox(this);
  var request = Components.
                classes["@mozilla.org/xmlextras/xmlhttprequest;1"].
                createInstance();

  request.open("GET", aUri, false);
  request.send(null);
  Components.utils.evalInSandbox(request.responseText, sandbox);

  return sandbox;
}

function FakeTimerService() {
  Cu.import("resource://weave/util.js");

  this.callbackQueue = [];

  var self = this;

  this.__proto__ = {
    makeTimerForCall: function FTS_makeTimerForCall(cb) {
      
      
      self.callbackQueue.push(cb);
      return "fake nsITimer";
    },
    processCallback: function FTS_processCallbacks() {
      var cb = self.callbackQueue.pop();
      if (cb) {
        cb();
        return true;
      }
      return false;
    }
  };

  Utils.makeTimerForCall = self.makeTimerForCall;
};

function getTestLogger(component) {
  return Log4Moz.Service.getLogger("Testing");
}

function initTestLogging(level) {
  Cu.import("resource://weave/log4moz.js");

  function LogStats() {
    this.errorsLogged = 0;
  }
  LogStats.prototype = {
    format: function BF_format(message) {
      if (message.level == Log4Moz.Level.Error)
        this.errorsLogged += 1;
      return message.loggerName + "\t" + message.levelDesc + "\t" +
        message.message + "\n";
    }
  };
  LogStats.prototype.__proto__ = new Log4Moz.Formatter();

  var log = Log4Moz.Service.rootLogger;
  var logStats = new LogStats();
  var appender = new Log4Moz.DumpAppender(logStats);

  if (typeof(level) == "undefined")
    level = "Debug";
  getTestLogger().level = Log4Moz.Level[level];

  log.level = Log4Moz.Level.Trace;
  appender.level = Log4Moz.Level.Trace;
  log.addAppender(appender);

  return logStats;
}

function makeAsyncTestRunner(generator) {
  Cu.import("resource://weave/async.js");

  var logStats = initTestLogging();

  function run_test() {
    do_test_pending();

    let onComplete = function() {
      if (logStats.errorsLogged)
        do_throw("Errors were logged.");
      else
        do_test_finished();
    };

    Async.run({}, generator, onComplete);
  }

  return run_test;
}

function FakePrefService(contents) {
  Cu.import("resource://weave/util.js");
  this.fakeContents = contents;
  Utils.__prefs = this;
}

FakePrefService.prototype = {
  _getPref: function fake__getPref(pref) {
    getTestLogger().trace("Getting pref: " + pref);
    return this.fakeContents[pref];
  },
  getCharPref: function fake_getCharPref(pref) {
    return this._getPref(pref);
  },
  getBoolPref: function fake_getBoolPref(pref) {
    return this._getPref(pref);
  },
  getIntPref: function fake_getIntPref(pref) {
    return this._getPref(pref);
  },
  addObserver: function fake_addObserver() {}
};

function makeFakeAsyncFunc(retval) {
  Cu.import("resource://weave/async.js");
  Function.prototype.async = Async.sugar;

  function fakeAsyncFunc() {
    let self = yield;

    Utils.makeTimerForCall(self.cb);
    yield;

    self.done(retval);
  }

  return fakeAsyncFunc;
}

function FakeDAVService(contents) {
  Cu.import("resource://weave/dav.js");

  this.fakeContents = contents;
  DAV.__proto__ = this;
  this.checkLogin = makeFakeAsyncFunc(true);
}

FakeDAVService.prototype = {
  PUT: function fake_PUT(path, data, onComplete) {
    getTestLogger().info("HTTP PUT to " + path + " with data: " + data);
    this.fakeContents[path] = data;
    makeFakeAsyncFunc({status: 200}).async(this, onComplete);
  },

  GET: function fake_GET(path, onComplete) {
    var result = {status: 404};
    if (path in this.fakeContents)
      result = {status: 200, responseText: this.fakeContents[path]};
    getTestLogger().info("HTTP GET from " + path + ", returning status " +
                         result.status);
    return makeFakeAsyncFunc(result).async(this, onComplete);
  },

  MKCOL: function fake_MKCOL(path, onComplete) {
    getTestLogger().info("HTTP MKCOL on " + path);
    makeFakeAsyncFunc(true).async(this, onComplete);
  }
};

function FakePasswordService(contents) {
  Cu.import("resource://weave/util.js");

  this.fakeContents = contents;
  let self = this;

  Utils.findPassword = function fake_findPassword(realm, username) {
    getTestLogger().trace("Password requested for " +
                          realm + ":" + username);
    if (realm in self.fakeContents && username in self.fakeContents[realm])
      return self.fakeContents[realm][username];
    else
      return null;
  };
};

function FakeFilesystemService(contents) {
  this.fakeContents = contents;

  let self = this;

  Utils.getProfileFile = function fake_getProfileFile(arg) {
    let fakeNsILocalFile = {
      exists: function() {
        return this._fakeFilename in self.fakeContents;
      },
      _fakeFilename: (typeof(arg) == "object") ? arg.path : arg
    };
    return fakeNsILocalFile;
  };

  Utils.readStream = function fake_readStream(stream) {
    getTestLogger().info("Reading from stream.");
    return stream._fakeData;
  };

  Utils.open = function fake_open(file, mode) {
    switch (mode) {
    case "<":
      mode = "reading";
      break;
    case ">":
      mode = "writing";
      break;
    default:
      throw new Error("Unexpected mode: " + mode);
    }

    getTestLogger().info("Opening '" + file._fakeFilename + "' for " +
                         mode + ".");
    var contents = "";
    if (file._fakeFilename in self.fakeContents && mode == "reading")
      contents = self.fakeContents[file._fakeFilename];
    let fakeStream = {
      writeString: function(data) {
        contents += data;
        getTestLogger().info("Writing data to local file '" +
                             file._fakeFilename +"': " + data);
      },
      close: function() {
        self.fakeContents[file._fakeFilename] = contents;
      },
      get _fakeData() { return contents; }
    };
    return [fakeStream];
  };
};

function FakeGUIDService() {
  let latestGUID = 0;

  Utils.makeGUID = function fake_makeGUID() {
    return "fake-guid-" + latestGUID++;
  };
}
