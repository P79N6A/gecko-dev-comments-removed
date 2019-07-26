


















"use strict";

this.EXPORTED_SYMBOLS = ["OS"];

const Cu = Components.utils;
const Ci = Components.interfaces;

let SharedAll = {};
Cu.import("resource://gre/modules/osfile/osfile_shared_allthreads.jsm", SharedAll);
Cu.import("resource://gre/modules/Deprecated.jsm", this);


let OS = SharedAll.OS;

let LOG = OS.Shared.LOG.bind(OS.Shared, "Controller");

let isTypedArray = OS.Shared.isTypedArray;


let OSError;
if (OS.Constants.Win) {
  Cu.import("resource://gre/modules/osfile/osfile_win_allthreads.jsm", this);
  OSError = OS.Shared.Win.Error;
} else if (OS.Constants.libc) {
  Cu.import("resource://gre/modules/osfile/osfile_unix_allthreads.jsm", this);
  OSError = OS.Shared.Unix.Error;
} else {
  throw new Error("I am neither under Windows nor under a Posix system");
}
let Type = OS.Shared.Type;
let Path = {};
Cu.import("resource://gre/modules/osfile/ospath.jsm", Path);


Cu.import("resource://gre/modules/Promise.jsm", this);


Cu.import("resource://gre/modules/osfile/_PromiseWorker.jsm", this);

Cu.import("resource://gre/modules/Services.jsm", this);

Cu.import("resource://gre/modules/AsyncShutdown.jsm", this);

LOG("Checking profileDir", OS.Constants.Path);



if (!("profileDir" in OS.Constants.Path)) {
  Object.defineProperty(OS.Constants.Path, "profileDir", {
    get: function() {
      let path = undefined;
      try {
        path = Services.dirsvc.get("ProfD", Ci.nsIFile).path;
        delete OS.Constants.Path.profileDir;
        OS.Constants.Path.profileDir = path;
      } catch (ex) {
        
      }
      return path;
    }
  });
}

LOG("Checking localProfileDir");

if (!("localProfileDir" in OS.Constants.Path)) {
  Object.defineProperty(OS.Constants.Path, "localProfileDir", {
    get: function() {
      let path = undefined;
      try {
        path = Services.dirsvc.get("ProfLD", Ci.nsIFile).path;
        delete OS.Constants.Path.localProfileDir;
        OS.Constants.Path.localProfileDir = path;
      } catch (ex) {
        
      }
      return path;
    }
  });
}




const noRefs = [];















let clone = function clone(object, refs = noRefs) {
  let result = {};
  
  let refer = function refer(result, key, object) {
    Object.defineProperty(result, key, {
        enumerable: true,
        get: function() {
            return object[key];
        },
        set: function(value) {
            object[key] = value;
        }
    });
  };
  for (let k in object) {
    if (refs.indexOf(k) < 0) {
      result[k] = object[k];
    } else {
      refer(result, k, object);
    }
  }
  return result;
};

let worker = new PromiseWorker(
  "resource://gre/modules/osfile/osfile_async_worker.js", LOG);
let Scheduler = {
  


  launched: false,

  



  shutdown: false,

  


  latestPromise: Promise.resolve("OS.File scheduler hasn't been launched yet"),

  post: function post(...args) {
    if (!this.launched && OS.Shared.DEBUG) {
      
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
        } else {
          throw error;
        }
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
    OS.Shared.DEBUG = readDebugPref(PREF_OSFILE_LOG, OS.Shared.DEBUG);
    if (Scheduler.launched) {
      
      Scheduler.post("SET_DEBUG", [OS.Shared.DEBUG]);
    }
  }, false);
OS.Shared.DEBUG = readDebugPref(PREF_OSFILE_LOG, false);

Services.prefs.addObserver(PREF_OSFILE_LOG_REDIRECT,
  function prefObserver(aSubject, aTopic, aData) {
    OS.Shared.TEST = readDebugPref(PREF_OSFILE_LOG_REDIRECT, OS.Shared.TEST);
  }, false);
OS.Shared.TEST = readDebugPref(PREF_OSFILE_LOG_REDIRECT, false);



if (OS.Shared.DEBUG && Scheduler.launched) {
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
    
    
    
    if (isTypedArray(buffer) && (!options || !("bytes" in options))) {
      
      
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
    
    
    
    if (isTypedArray(buffer) && (!options || !("bytes" in options))) {
      
      
      options = clone(options, ["outExecutionDuration"]);
      options.bytes = buffer.byteLength;
    }
    
    
    
    
    return Scheduler.post("File_prototype_write",
      [this._fdmsg,
       Type.void_t.in_ptr.toMsg(buffer),
       options],
       buffer);
  },

  








  read: function read(nbytes) {
    let promise = Scheduler.post("File_prototype_read",
      [this._fdmsg,
       nbytes]);
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







File.Info = function Info(value) {
  
  
  for (let k in value) {
    if (k != "creationDate") {
      Object.defineProperty(this, k, {value: value[k]});
    }
  }
  Object.defineProperty(this, "_deprecatedCreationDate", {value: value["creationDate"]});
};
if (OS.Constants.Win) {
  File.Info.prototype = Object.create(OS.Shared.Win.AbstractInfo.prototype);
} else if (OS.Constants.libc) {
  File.Info.prototype = Object.create(OS.Shared.Unix.AbstractInfo.prototype);
} else {
  throw new Error("I am neither under Windows nor under a Posix system");
}


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
        
        
        if (!(reason instanceof WorkerErrorEvent && reason.message == "uncaught exception: [object StopIteration]")) {
          
          throw reason;
        }
        self.close();
        throw StopIteration;
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
if (OS.Constants.Win) {
  DirectoryIterator.Entry.prototype = Object.create(OS.Shared.Win.AbstractEntry.prototype);
} else if (OS.Constants.libc) {
  DirectoryIterator.Entry.prototype = Object.create(OS.Shared.Unix.AbstractEntry.prototype);
} else {
  throw new Error("I am neither under Windows nor under a Posix system");
}

DirectoryIterator.Entry.fromMsg = function fromMsg(value) {
  return new DirectoryIterator.Entry(value);
};


Object.defineProperty(File, "POS_START", {value: OS.Shared.POS_START});
Object.defineProperty(File, "POS_CURRENT", {value: OS.Shared.POS_CURRENT});
Object.defineProperty(File, "POS_END", {value: OS.Shared.POS_END});

OS.File = File;
OS.File.Error = OSError;
OS.File.DirectoryIterator = DirectoryIterator;
OS.Path = Path;






AsyncShutdown.profileBeforeChange.addBlocker(
  "OS.File: flush I/O queued before profile-before-change",
  () =>
    
    Scheduler.latestPromise.then(null,
      function onError() { })
);
