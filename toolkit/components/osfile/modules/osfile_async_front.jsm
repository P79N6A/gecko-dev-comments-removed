


















"use strict";

this.EXPORTED_SYMBOLS = ["OS"];

const Cu = Components.utils;
const Ci = Components.interfaces;

let SharedAll = {};
Cu.import("resource://gre/modules/osfile/osfile_shared_allthreads.jsm", SharedAll);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Timer.jsm", this);

XPCOMUtils.defineLazyModuleGetter(this, 'Deprecated',
  'resource://gre/modules/Deprecated.jsm');


let LOG = SharedAll.LOG.bind(SharedAll, "Controller");
let isTypedArray = SharedAll.isTypedArray;


let SysAll = {};
if (SharedAll.Constants.Win) {
  Cu.import("resource://gre/modules/osfile/osfile_win_allthreads.jsm", SysAll);
} else if (SharedAll.Constants.libc) {
  Cu.import("resource://gre/modules/osfile/osfile_unix_allthreads.jsm", SysAll);
} else {
  throw new Error("I am neither under Windows nor under a Posix system");
}
let OSError = SysAll.Error;
let Type = SysAll.Type;

let Path = {};
Cu.import("resource://gre/modules/osfile/ospath.jsm", Path);


Cu.import("resource://gre/modules/Promise.jsm", this);
Cu.import("resource://gre/modules/Task.jsm", this);


Cu.import("resource://gre/modules/osfile/_PromiseWorker.jsm", this);

Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/TelemetryStopwatch.jsm", this);
Cu.import("resource://gre/modules/AsyncShutdown.jsm", this);
let Native = Cu.import("resource://gre/modules/osfile/osfile_native.jsm", {});





const EXCEPTION_CONSTRUCTORS = {
  EvalError: function(error) {
    return new EvalError(error.message, error.fileName, error.lineNumber);
  },
  InternalError: function(error) {
    return new InternalError(error.message, error.fileName, error.lineNumber);
  },
  RangeError: function(error) {
    return new RangeError(error.message, error.fileName, error.lineNumber);
  },
  ReferenceError: function(error) {
    return new ReferenceError(error.message, error.fileName, error.lineNumber);
  },
  SyntaxError: function(error) {
    return new SyntaxError(error.message, error.fileName, error.lineNumber);
  },
  TypeError: function(error) {
    return new TypeError(error.message, error.fileName, error.lineNumber);
  },
  URIError: function(error) {
    return new URIError(error.message, error.fileName, error.lineNumber);
  },
  OSError: function(error) {
    return OS.File.Error.fromMsg(error);
  }
};





function lazyPathGetter(constProp, dirKey) {
  return function () {
    let path;
    try {
      path = Services.dirsvc.get(dirKey, Ci.nsIFile).path;
      delete SharedAll.Constants.Path[constProp];
      SharedAll.Constants.Path[constProp] = path;
    } catch (ex) {
      
      
    }

    return path;
  };
}

for (let [constProp, dirKey] of [
  ["localProfileDir", "ProfLD"],
  ["profileDir", "ProfD"],
  ["userApplicationDataDir", "UAppData"],
  ["winAppDataDir", "AppData"],
  ["winStartMenuProgsDir", "Progs"],
  ]) {

  if (constProp in SharedAll.Constants.Path) {
    continue;
  }

  LOG("Installing lazy getter for OS.Constants.Path." + constProp +
      " because it isn't defined and profile may not be loaded.");
  Object.defineProperty(SharedAll.Constants.Path, constProp, {
    get: lazyPathGetter(constProp, dirKey),
  });
}




let clone = SharedAll.clone;












function summarizeObject(obj) {
  if (!obj) {
    return null;
  }
  if (typeof obj == "string") {
    if (obj.length > 1024) {
      return {"Long string": obj.length};
    }
    return obj;
  }
  if (typeof obj == "object") {
    if (Array.isArray(obj)) {
      if (obj.length > 32) {
        return {"Long array": obj.length};
      }
      return [summarizeObject(k) for (k of obj)];
    }
    if ("byteLength" in obj) {
      
      return {"Binary Data": obj.byteLength};
    }
    let result = {};
    for (let k of Object.keys(obj)) {
      result[k] = summarizeObject(obj[k]);
    }
    return result;
  }
  return obj;
}

let worker = null;
let Scheduler = {
  



  launched: false,

  



  shutdown: false,

  




  queue: Promise.resolve(),

  


  Debugging: {
    


    latestSent: undefined,

    


    latestReceived: undefined,

    



    messagesSent: 0,

    



    messagesQueued: 0,

    


    messagesReceived: 0,
  },

  


  resetTimer: null,

  restartTimer: function(arg) {
    let delay;
    try {
      delay = Services.prefs.getIntPref("osfile.reset_worker_delay");
    } catch(e) {
      
      return;
    }

    if (this.resetTimer) {
      clearTimeout(this.resetTimer);
    }
    this.resetTimer = setTimeout(File.resetWorker, delay);
  },

  








  push: function(code) {
    let promise = this.queue.then(code);
    
    this.queue = promise.then(null, () => undefined);
    
    return promise.then(null, null);
  },

  








  post: function post(method, ...args) {
    if (this.shutdown) {
      LOG("OS.File is not available anymore. The following request has been rejected.",
        method, args);
      return Promise.reject(new Error("OS.File has been shut down."));
    }
    if (!worker) {
      
      worker = new PromiseWorker(
        "resource://gre/modules/osfile/osfile_async_worker.js", LOG);
    }
    let firstLaunch = !this.launched;
    this.launched = true;

    if (firstLaunch && SharedAll.Config.DEBUG) {
      
      worker.post("SET_DEBUG", [true]);
      Scheduler.Debugging.messagesSent++;
    }

    
    let options;
    let methodArgs = args[0];
    if (methodArgs) {
      options = methodArgs[methodArgs.length - 1];
    }
    Scheduler.Debugging.messagesQueued++;
    return this.push(() => Task.spawn(function*() {
      
      
      Scheduler.Debugging.latestReceived = null;
      Scheduler.Debugging.latestSent = [Date.now(), method, summarizeObject(methodArgs)];
      let data;
      let reply;
      let isError = false;
      try {
        try {
          data = yield worker.post(method, ...args);
        } finally {
          Scheduler.Debugging.messagesReceived++;
        }
        reply = data;
      } catch (error) {
        reply = error;
        isError = true;
        if (error instanceof PromiseWorker.WorkerError) {
          throw EXCEPTION_CONSTRUCTORS[error.data.exn || "OSError"](error.data);
        }
        if (error instanceof ErrorEvent) {
          let message = error.message;
          if (message == "uncaught exception: [object StopIteration]") {
            isError = false;
            throw StopIteration;
          }
          throw new Error(message, error.filename, error.lineno);
        }
        throw error;
      } finally {
        Scheduler.Debugging.latestSent = Scheduler.Debugging.latestSent.slice(0, 2);
        if (isError) {
          Scheduler.Debugging.latestReceived = [Date.now(), reply.message, reply.fileName, reply.lineNumber];
        } else {
          Scheduler.Debugging.latestReceived = [Date.now(), summarizeObject(reply)];
        }
        if (firstLaunch) {
          Scheduler._updateTelemetry();
        }

        
        
        if (method != "Meta_reset") {
          Scheduler.restartTimer();
        }
      }

      
      if (!options) {
        return data.ok;
      }
      
      if (typeof options !== "object" ||
        !("outExecutionDuration" in options)) {
        return data.ok;
      }
      
      
      if (!("durationMs" in data)) {
        return data.ok;
      }
      
      
      
      
      let durationMs = Math.max(0, data.durationMs);
      
      if (typeof options.outExecutionDuration == "number") {
        options.outExecutionDuration += durationMs;
      } else {
        options.outExecutionDuration = durationMs;
      }
      return data.ok;
    }));
  },

  




  _updateTelemetry: function() {
    let workerTimeStamps = worker.workerTimeStamps;
    if (!workerTimeStamps) {
      
      
      
      return;
    }
    let HISTOGRAM_LAUNCH = Services.telemetry.getHistogramById("OSFILE_WORKER_LAUNCH_MS");
    HISTOGRAM_LAUNCH.add(worker.workerTimeStamps.entered - worker.launchTimeStamp);

    let HISTOGRAM_READY = Services.telemetry.getHistogramById("OSFILE_WORKER_READY_MS");
    HISTOGRAM_READY.add(worker.workerTimeStamps.loaded - worker.launchTimeStamp);
  }
};

const PREF_OSFILE_LOG = "toolkit.osfile.log";
const PREF_OSFILE_LOG_REDIRECT = "toolkit.osfile.log.redirect";








function readDebugPref(prefName, oldPref = false) {
  let pref = oldPref;
  try {
    pref = Services.prefs.getBoolPref(prefName);
  } catch (x) {
    
  }
  
  return pref;
};





Services.prefs.addObserver(PREF_OSFILE_LOG,
  function prefObserver(aSubject, aTopic, aData) {
    SharedAll.Config.DEBUG = readDebugPref(PREF_OSFILE_LOG, SharedAll.Config.DEBUG);
    if (Scheduler.launched) {
      
      Scheduler.post("SET_DEBUG", [SharedAll.Config.DEBUG]);
    }
  }, false);
SharedAll.Config.DEBUG = readDebugPref(PREF_OSFILE_LOG, false);

Services.prefs.addObserver(PREF_OSFILE_LOG_REDIRECT,
  function prefObserver(aSubject, aTopic, aData) {
    SharedAll.Config.TEST = readDebugPref(PREF_OSFILE_LOG_REDIRECT, OS.Shared.TEST);
  }, false);
SharedAll.Config.TEST = readDebugPref(PREF_OSFILE_LOG_REDIRECT, false);






let nativeWheneverAvailable = true;
const PREF_OSFILE_NATIVE = "toolkit.osfile.native";
Services.prefs.addObserver(PREF_OSFILE_NATIVE,
  function prefObserver(aSubject, aTopic, aData) {
    nativeWheneverAvailable = readDebugPref(PREF_OSFILE_NATIVE, nativeWheneverAvailable);
  }, false);




if (SharedAll.Config.DEBUG && Scheduler.launched) {
  Scheduler.post("SET_DEBUG", [true]);
}


const WEB_WORKERS_SHUTDOWN_TOPIC = "web-workers-shutdown";


const PREF_OSFILE_TEST_SHUTDOWN_OBSERVER =
  "toolkit.osfile.test.shutdown.observer";












function warnAboutUnclosedFiles(shutdown = true) {
  if (!Scheduler.launched || !worker) {
    
    
    
    return null;
  }
  let promise = Scheduler.post("Meta_getUnclosedResources");

  
  if (shutdown) {
    Scheduler.shutdown = true;
  }

  return promise.then(function onSuccess(opened) {
    let msg = "";
    if (opened.openedFiles.length > 0) {
      msg += "The following files are still open:\n" +
        opened.openedFiles.join("\n");
    }
    if (msg) {
      msg += "\n";
    }
    if (opened.openedDirectoryIterators.length > 0) {
      msg += "The following directory iterators are still open:\n" +
        opened.openedDirectoryIterators.join("\n");
    }
    
    if (msg) {
      LOG("WARNING: File descriptors leaks detected.\n" + msg);
    }
  });
};

AsyncShutdown.webWorkersShutdown.addBlocker(
  "OS.File: flush pending requests, warn about unclosed files, shut down service.",
  () => warnAboutUnclosedFiles(true)
);






Services.prefs.addObserver(PREF_OSFILE_TEST_SHUTDOWN_OBSERVER,
  function prefObserver() {
    
    
    let TOPIC = null;
    try {
      TOPIC = Services.prefs.getCharPref(
        PREF_OSFILE_TEST_SHUTDOWN_OBSERVER);
    } catch (x) {
    }
    if (TOPIC) {
      
      
      
      let phase = AsyncShutdown._getPhase(TOPIC);
      phase.addBlocker(
        "(for testing purposes) OS.File: warn about unclosed files",
        () => warnAboutUnclosedFiles(false)
      );
    }
  }, false);









let File = function File(fdmsg) {
  
  
  this._fdmsg = fdmsg;
  this._closeResult = null;
  this._closed = null;
};


File.prototype = {
  








  close: function close() {
    if (this._fdmsg != null) {
      let msg = this._fdmsg;
      this._fdmsg = null;
      return this._closeResult =
        Scheduler.post("File_prototype_close", [msg], this);
    }
    return this._closeResult;
  },

  






  stat: function stat() {
    return Scheduler.post("File_prototype_stat", [this._fdmsg], this).then(
      File.Info.fromMsg
    );
  },

  








  setDates: function setDates(accessDate, modificationDate) {
    return Scheduler.post("File_prototype_setDates",
                          [this._fdmsg, accessDate, modificationDate], this);
  },

  











  readTo: function readTo(buffer, options = {}) {
    
    
    
    
    
    if (isTypedArray(buffer) && !(options && "bytes" in options)) {
      
      options = clone(options, ["outExecutionDuration"]);
      options.bytes = buffer.byteLength;
    }
    
    
    
    
    return Scheduler.post("File_prototype_readTo",
      [this._fdmsg,
       Type.void_t.out_ptr.toMsg(buffer),
       options],
       buffer);
  },
  

















  write: function write(buffer, options = {}) {
    
    
    
    
    
    if (isTypedArray(buffer) && !(options && "bytes" in options)) {
      
      options = clone(options, ["outExecutionDuration"]);
      options.bytes = buffer.byteLength;
    }
    
    
    
    
    return Scheduler.post("File_prototype_write",
      [this._fdmsg,
       Type.void_t.in_ptr.toMsg(buffer),
       options],
       buffer);
  },

  









  read: function read(nbytes, options = {}) {
    let promise = Scheduler.post("File_prototype_read",
      [this._fdmsg,
       nbytes, options]);
    return promise.then(
      function onSuccess(data) {
        return new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
      });
  },

  






  getPosition: function getPosition() {
    return Scheduler.post("File_prototype_getPosition",
      [this._fdmsg]);
  },

  










  setPosition: function setPosition(pos, whence) {
    return Scheduler.post("File_prototype_setPosition",
      [this._fdmsg, pos, whence]);
  },

  










  flush: function flush() {
    return Scheduler.post("File_prototype_flush",
      [this._fdmsg]);
  }
};








File.open = function open(path, mode, options) {
  return Scheduler.post(
    "open", [Type.path.toMsg(path), mode, options],
    path
  ).then(
    function onSuccess(msg) {
      return new File(msg);
    }
  );
};
















File.openUnique = function openUnique(path, options) {
  return Scheduler.post(
      "openUnique", [Type.path.toMsg(path), options],
      path
    ).then(
    function onSuccess(msg) {
      return {
        path: msg.path,
        file: new File(msg.file)
      };
    }
  );
};








File.stat = function stat(path, options) {
  return Scheduler.post(
    "stat", [Type.path.toMsg(path), options],
    path).then(File.Info.fromMsg);
};











File.setDates = function setDates(path, accessDate, modificationDate) {
  return Scheduler.post("setDates",
                        [Type.path.toMsg(path), accessDate, modificationDate],
                        this);
};








File.getCurrentDirectory = function getCurrentDirectory() {
  return Scheduler.post(
    "getCurrentDirectory"
  ).then(Type.path.fromMsg);
};












File.setCurrentDirectory = function setCurrentDirectory(path) {
  return Scheduler.post(
    "setCurrentDirectory", [Type.path.toMsg(path)], path
  );
};
























File.copy = function copy(sourcePath, destPath, options) {
  return Scheduler.post("copy", [Type.path.toMsg(sourcePath),
    Type.path.toMsg(destPath), options], [sourcePath, destPath]);
};

























File.move = function move(sourcePath, destPath, options) {
  return Scheduler.post("move", [Type.path.toMsg(sourcePath),
    Type.path.toMsg(destPath), options], [sourcePath, destPath]);
};












if (!SharedAll.Constants.Win) {
  File.unixSymLink = function unixSymLink(sourcePath, destPath) {
    return Scheduler.post("unixSymLink", [Type.path.toMsg(sourcePath),
      Type.path.toMsg(destPath)], [sourcePath, destPath]);
  };
}










File.getAvailableFreeSpace = function getAvailableFreeSpace(sourcePath) {
  return Scheduler.post("getAvailableFreeSpace",
    [Type.path.toMsg(sourcePath)], sourcePath
  ).then(Type.uint64_t.fromMsg);
};









File.removeEmptyDir = function removeEmptyDir(path, options) {
  return Scheduler.post("removeEmptyDir",
    [Type.path.toMsg(path), options], path);
};






File.remove = function remove(path) {
  return Scheduler.post("remove",
    [Type.path.toMsg(path)]);
};

















File.makeDir = function makeDir(path, options) {
  return Scheduler.post("makeDir",
    [Type.path.toMsg(path), options], path);
};




















File.read = function read(path, bytes, options = {}) {
  if (typeof bytes == "object") {
    
    
    options = bytes || {};
  } else {
    options = clone(options, ["outExecutionDuration"]);
    if (typeof bytes != "undefined") {
      options.bytes = bytes;
    }
  }

  if (options.compression || !nativeWheneverAvailable) {
    
    let promise = Scheduler.post("read",
      [Type.path.toMsg(path), bytes, options], path);
    return promise.then(
      function onSuccess(data) {
        if (typeof data == "string") {
          return data;
        }
        return new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
      });
  }

  
  return Scheduler.push(() => Native.read(path, options));
};








File.exists = function exists(path) {
  return Scheduler.post("exists",
    [Type.path.toMsg(path)], path);
};













































File.writeAtomic = function writeAtomic(path, buffer, options = {}) {
  
  
  options = clone(options, ["outExecutionDuration"]);
  
  if ("tmpPath" in options) {
    options.tmpPath = Type.path.toMsg(options.tmpPath);
  };
  if (isTypedArray(buffer) && (!("bytes" in options))) {
    options.bytes = buffer.byteLength;
  };
  
  
  
  
  let refObj = {};
  TelemetryStopwatch.start("OSFILE_WRITEATOMIC_JANK_MS", refObj);
  let promise = Scheduler.post("writeAtomic",
    [Type.path.toMsg(path),
     Type.void_t.in_ptr.toMsg(buffer),
     options], [options, buffer]);
  TelemetryStopwatch.finish("OSFILE_WRITEATOMIC_JANK_MS", refObj);
  return promise;
};

File.removeDir = function(path, options = {}) {
  return Scheduler.post("removeDir",
    [Type.path.toMsg(path), options], path);
};







File.Info = function Info(value) {
  
  
  for (let k in value) {
    if (k != "creationDate") {
      Object.defineProperty(this, k, {value: value[k]});
    }
  }
  Object.defineProperty(this, "_deprecatedCreationDate", {value: value["creationDate"]});
};
File.Info.prototype = SysAll.AbstractInfo.prototype;


Object.defineProperty(File.Info.prototype, "creationDate", {
  get: function creationDate() {
    Deprecated.warning("Field 'creationDate' is deprecated.", "https://developer.mozilla.org/en-US/docs/JavaScript_OS.File/OS.File.Info#Cross-platform_Attributes");
    return this._deprecatedCreationDate;
  }
});

File.Info.fromMsg = function fromMsg(value) {
  return new File.Info(value);
};





File.GET_DEBUG = function GET_DEBUG() {
  return Scheduler.post("GET_DEBUG");
};






let DirectoryIterator = function DirectoryIterator(path, options) {
  








  this.__itmsg = Scheduler.post(
    "new_DirectoryIterator", [Type.path.toMsg(path), options],
    path
  );
  this._isClosed = false;
};
DirectoryIterator.prototype = {
  iterator: function () this,
  __iterator__: function () this,

  
  
  
  
  get _itmsg() {
    if (!this.__itmsg) {
      this.__itmsg = Promise.reject(StopIteration);
    }
    return this.__itmsg;
  },

  




  exists: function exists() {
    return this._itmsg.then(
      function onSuccess(iterator) {
        return Scheduler.post("DirectoryIterator_prototype_exists", [iterator]);
      }
    );
  },
  






  next: function next() {
    let self = this;
    let promise = this._itmsg;

    
    promise = promise.then(
      function withIterator(iterator) {
        return self._next(iterator);
      });

    return promise;
  },
  







  nextBatch: function nextBatch(size) {
    if (this._isClosed) {
      return Promise.resolve([]);
    }
    let promise = this._itmsg;
    promise = promise.then(
      function withIterator(iterator) {
        return Scheduler.post("DirectoryIterator_prototype_nextBatch", [iterator, size]);
      });
    promise = promise.then(
      function withEntries(array) {
        return array.map(DirectoryIterator.Entry.fromMsg);
      });
    return promise;
  },
  














  forEach: function forEach(cb, options) {
    if (this._isClosed) {
      return Promise.resolve();
    }

    let self = this;
    let position = 0;
    let iterator;

    
    let promise = this._itmsg.then(
      function(aIterator) {
        iterator = aIterator;
      }
    );

    
    let loop = function loop() {
      if (self._isClosed) {
        return Promise.resolve();
      }
      return self._next(iterator).then(
        function onSuccess(value) {
          return Promise.resolve(cb(value, position++, self)).then(loop);
        },
        function onFailure(reason) {
          if (reason == StopIteration) {
            return;
          }
          throw reason;
        }
      );
    };

    return promise.then(loop);
  },
  





  _next: function _next(iterator) {
    if (this._isClosed) {
      return this._itmsg;
    }
    let self = this;
    let promise = Scheduler.post("DirectoryIterator_prototype_next", [iterator]);
    promise = promise.then(
      DirectoryIterator.Entry.fromMsg,
      function onReject(reason) {
        if (reason == StopIteration) {
          self.close();
          throw StopIteration;
        }
        throw reason;
      });
    return promise;
  },
  


  close: function close() {
    if (this._isClosed) {
      return Promise.resolve();
    }
    this._isClosed = true;
    let self = this;
    return this._itmsg.then(
      function withIterator(iterator) {
        
        
        self.__itmsg = null;
        return Scheduler.post("DirectoryIterator_prototype_close", [iterator]);
      }
    );
  }
};

DirectoryIterator.Entry = function Entry(value) {
  return value;
};
DirectoryIterator.Entry.prototype = Object.create(SysAll.AbstractEntry.prototype);

DirectoryIterator.Entry.fromMsg = function fromMsg(value) {
  return new DirectoryIterator.Entry(value);
};









File.resetWorker = function() {
  if (!Scheduler.launched || Scheduler.shutdown) {
    
    return Promise.resolve();
  }
  return Scheduler.post("Meta_reset").then(
    function(wouldLeak) {
      if (!wouldLeak) {
        
        worker = null;
        return;
      }
      
      
      let msg = "Cannot reset worker: ";
      let {openedFiles, openedDirectoryIterators} = wouldLeak;
      if (openedFiles.length > 0) {
        msg += "The following files are still open:\n" +
          openedFiles.join("\n");
      }
      if (openedDirectoryIterators.length > 0) {
        msg += "The following directory iterators are still open:\n" +
          openedDirectoryIterators.join("\n");
      }
      throw new Error(msg);
    }
  );
};



File.POS_START = SysAll.POS_START;
File.POS_CURRENT = SysAll.POS_CURRENT;
File.POS_END = SysAll.POS_END;


File.Error = OSError;
File.DirectoryIterator = DirectoryIterator;

this.OS = {};
this.OS.File = File;
this.OS.Constants = SharedAll.Constants;
this.OS.Shared = {
  LOG: SharedAll.LOG,
  Type: SysAll.Type,
  get DEBUG() {
    return SharedAll.Config.DEBUG;
  },
  set DEBUG(x) {
    return SharedAll.Config.DEBUG = x;
  }
};
Object.freeze(this.OS.Shared);
this.OS.Path = Path;






AsyncShutdown.profileBeforeChange.addBlocker(
  "OS.File: flush I/O queued before profile-before-change",
  
  function() {
    let DEBUG = false;
    try {
      DEBUG = Services.prefs.getBoolPref("toolkit.osfile.debug.failshutdown");
    } catch (ex) {
      
    }
    if (DEBUG) {
      
      return Promise.defer().promise;
    } else {
      return Scheduler.queue;
    }
  },
  function getDetails() {
    let result = {
      launched: Scheduler.launched,
      shutdown: Scheduler.shutdown,
      worker: !!worker,
      pendingReset: !!Scheduler.resetTimer,
      latestSent: Scheduler.Debugging.latestSent,
      latestReceived: Scheduler.Debugging.latestReceived,
      messagesSent: Scheduler.Debugging.messagesSent,
      messagesReceived: Scheduler.Debugging.messagesReceived,
      messagesQueued: Scheduler.Debugging.messagesQueued,
      DEBUG: SharedAll.Config.DEBUG
    };
    
    for (let key of ["latestSent", "latestReceived"]) {
      if (result[key] && typeof result[key][0] == "number") {
        result[key][0] = Date(result[key][0]);
      }
    }
    return result;
  }
);
