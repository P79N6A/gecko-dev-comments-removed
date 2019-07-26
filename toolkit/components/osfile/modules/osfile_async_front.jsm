


















"use strict";

this.EXPORTED_SYMBOLS = ["OS"];

const Cu = Components.utils;
const Ci = Components.interfaces;

let SharedAll = {};
Cu.import("resource://gre/modules/osfile/osfile_shared_allthreads.jsm", SharedAll);
Cu.import("resource://gre/modules/Deprecated.jsm", this);


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


Cu.import("resource://gre/modules/osfile/_PromiseWorker.jsm", this);

Cu.import("resource://gre/modules/Services.jsm", this);

Cu.import("resource://gre/modules/AsyncShutdown.jsm", this);



if (!("profileDir" in SharedAll.Constants.Path)) {
  Object.defineProperty(SharedAll.Constants.Path, "profileDir", {
    get: function() {
      let path = undefined;
      try {
        path = Services.dirsvc.get("ProfD", Ci.nsIFile).path;
        delete SharedAll.Constants.Path.profileDir;
        SharedAll.Constants.Path.profileDir = path;
      } catch (ex) {
        
      }
      return path;
    }
  });
}

LOG("Checking localProfileDir");

if (!("localProfileDir" in SharedAll.Constants.Path)) {
  Object.defineProperty(SharedAll.Constants.Path, "localProfileDir", {
    get: function() {
      let path = undefined;
      try {
        path = Services.dirsvc.get("ProfLD", Ci.nsIFile).path;
        delete SharedAll.Constants.Path.localProfileDir;
        SharedAll.Constants.Path.localProfileDir = path;
      } catch (ex) {
        
      }
      return path;
    }
  });
}




let clone = SharedAll.clone;

let worker = new PromiseWorker(
  "resource://gre/modules/osfile/osfile_async_worker.js", LOG);
let Scheduler = {
  


  launched: false,

  



  shutdown: false,

  


  latestPromise: Promise.resolve("OS.File scheduler hasn't been launched yet"),

  post: function post(...args) {
    if (!this.launched && SharedAll.Config.DEBUG) {
      
      worker.post("SET_DEBUG", [true]);
    }
    this.launched = true;
    if (this.shutdown) {
      LOG("OS.File is not available anymore. The following request has been rejected.", args);
      return Promise.reject(new Error("OS.File has been shut down."));
    }

    
    let methodArgs = args[1];
    let options = methodArgs ? methodArgs[methodArgs.length - 1] : null;
    let promise = worker.post.apply(worker, args);
    return this.latestPromise = promise.then(
      function onSuccess(data) {
        
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
      },
      function onError(error) {
        
        if (error instanceof PromiseWorker.WorkerError) {
          throw OS.File.Error.fromMsg(error.data);
        }
        
        if (typeof error == "object" && error && error.constructor.name == "WorkerErrorEvent") {
          let message = error.message;
          if (message == "uncaught exception: [object StopIteration]") {
            throw StopIteration;
          }
          throw new Error(message, error.filename, error.lineno);
        }
        throw error;
      }
    );
  }
};

const PREF_OSFILE_LOG = "toolkit.osfile.log";
const PREF_OSFILE_LOG_REDIRECT = "toolkit.osfile.log.redirect";








let readDebugPref = function readDebugPref(prefName, oldPref = false) {
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



if (SharedAll.Config.DEBUG && Scheduler.launched) {
  Scheduler.post("SET_DEBUG", [true]);
}


const WEB_WORKERS_SHUTDOWN_TOPIC = "web-workers-shutdown";


const PREF_OSFILE_TEST_SHUTDOWN_OBSERVER =
  "toolkit.osfile.test.shutdown.observer";












function warnAboutUnclosedFiles(shutdown = true) {
  if (!Scheduler.launched) {
    
    
    
    return null;
  }
  
  let promise = Scheduler.post("System_shutdown");

  
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

  











  readTo: function readTo(buffer, options = {}) {
    
    
    
    if (isTypedArray(buffer) && !("bytes" in options)) {
      
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
    
    
    
    if (isTypedArray(buffer)) {
      
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








File.stat = function stat(path) {
  return Scheduler.post(
    "stat", [Type.path.toMsg(path)],
    path).then(File.Info.fromMsg);
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

















File.read = function read(path, bytes, options) {
  let promise = Scheduler.post("read",
    [Type.path.toMsg(path), bytes, options], path);
  return promise.then(
    function onSuccess(data) {
      return new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
    });
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
  
  
  
  
  return Scheduler.post("writeAtomic",
    [Type.path.toMsg(path),
     Type.void_t.in_ptr.toMsg(buffer),
     options], [options, buffer]);
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
  








  this._itmsg = Scheduler.post(
    "new_DirectoryIterator", [Type.path.toMsg(path), options],
    path
  );
  this._isClosed = false;
};
DirectoryIterator.prototype = {
  iterator: function () this,
  __iterator__: function () this,

  




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
        self._itmsg = Promise.reject(StopIteration);
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


File.POS_START = SysAll.POS_START;
File.POS_CURRENT = SysAll.POS_CURRENT;
File.POS_END = SysAll.POS_END;


File.Error = OSError;
File.DirectoryIterator = DirectoryIterator;

this.OS = {};
OS.File = File;
OS.Constants = SharedAll.Constants;
OS.Shared = {
  LOG: SharedAll.LOG,
  Type: SysAll.Type,
  get DEBUG() {
    return SharedAll.Config.DEBUG;
  },
  set DEBUG(x) {
    return SharedAll.Config.DEBUG = x;
  }
};
Object.freeze(OS.Shared);
OS.Path = Path;






AsyncShutdown.profileBeforeChange.addBlocker(
  "OS.File: flush I/O queued before profile-before-change",
  () =>
    
    Scheduler.latestPromise.then(null,
      function onError() { })
);
