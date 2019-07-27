


















"use strict";

this.EXPORTED_SYMBOLS = ["OS"];

const Cu = Components.utils;
const Ci = Components.interfaces;

let SharedAll = {};
Cu.import("resource://gre/modules/osfile/osfile_shared_allthreads.jsm", SharedAll);
Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/Timer.jsm", this);



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


Cu.import("resource://gre/modules/PromiseWorker.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/TelemetryStopwatch.jsm", this);
Cu.import("resource://gre/modules/AsyncShutdown.jsm", this);
let Native = Cu.import("resource://gre/modules/osfile/osfile_native.jsm", {});






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




let Scheduler = this.Scheduler = {

  



  launched: false,

  



  shutdown: false,

  




  queue: Promise.resolve(),

  





  _killQueue: Promise.resolve(),

  


  Debugging: {
    


    latestSent: undefined,

    


    latestReceived: undefined,

    



    messagesSent: 0,

    



    messagesQueued: 0,

    


    messagesReceived: 0,
  },

  


  resetTimer: null,

  







  get worker() {
    if (!this._worker) {
      
      
      this._worker = new BasePromiseWorker("resource://gre/modules/osfile/osfile_async_worker.js");
      this._worker.log = LOG;
      this._worker.ExceptionHandlers["OS.File.Error"] = OSError.fromMsg;
    }
    return this._worker;
  },

  _worker: null,

  


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
    this.resetTimer = setTimeout(
      () => Scheduler.kill({reset: true, shutdown: false}),
      delay
    );
  },

  









  kill: function({shutdown, reset}) {
    
    
    let killQueue = this._killQueue;

    
    
    
    
    let deferred = Promise.defer();
    let savedQueue = this.queue;
    this.queue = deferred.promise;

    return this._killQueue = Task.spawn(function*() {

      yield killQueue;
      
      

      yield savedQueue;

      try {
        
        
        

        if (!this.launched || this.shutdown || !this._worker) {
          
          this.shutdown = this.shutdown || shutdown;
          this._worker = null;
          return null;
        }

        

        let message = ["Meta_shutdown", [reset]];

        Scheduler.latestReceived = [];
        Scheduler.latestSent = [Date.now(), ...message];

        
        let resources;
        try {
          resources = yield this._worker.post(...message);

          Scheduler.latestReceived = [Date.now(), message];
        } catch (ex) {
          LOG("Could not dispatch Meta_reset", ex);
          
          
          
          resources = {openedFiles: [], openedDirectoryIterators: [], killed: true};

          Scheduler.latestReceived = [Date.now(), message, ex];
        }

        let {openedFiles, openedDirectoryIterators, killed} = resources;
        if (!reset
          && (openedFiles && openedFiles.length
            || ( openedDirectoryIterators && openedDirectoryIterators.length))) {
          

          let msg = "";
          if (openedFiles.length > 0) {
            msg += "The following files are still open:\n" +
              openedFiles.join("\n");
          }
          if (openedDirectoryIterators.length > 0) {
            msg += "The following directory iterators are still open:\n" +
              openedDirectoryIterators.join("\n");
          }

          LOG("WARNING: File descriptors leaks detected.\n" + msg);
        }

        
        if (killed || shutdown) {
          this._worker = null;
        }

        this.shutdown = shutdown;

        return resources;

      } finally {
        
        
        
        deferred.resolve();
      }

    }.bind(this));
  },

  








  push: function(code) {
    let promise = this.queue.then(code);
    
    this.queue = promise.then(null, () => undefined);
    
    return promise.then(null, null);
  },

  








  post: function post(method, args = undefined, closure = undefined) {
    if (this.shutdown) {
      LOG("OS.File is not available anymore. The following request has been rejected.",
        method, args);
      return Promise.reject(new Error("OS.File has been shut down. Rejecting post to " + method));
    }
    let firstLaunch = !this.launched;
    this.launched = true;

    if (firstLaunch && SharedAll.Config.DEBUG) {
      
      this.worker.post("SET_DEBUG", [true]);
      Scheduler.Debugging.messagesSent++;
    }

    Scheduler.Debugging.messagesQueued++;
    return this.push(Task.async(function*() {
      if (this.shutdown) {
	LOG("OS.File is not available anymore. The following request has been rejected.",
	  method, args);
	throw new Error("OS.File has been shut down. Rejecting request to " + method);
      }

      
      
      Scheduler.Debugging.latestReceived = null;
      Scheduler.Debugging.latestSent = [Date.now(), method, summarizeObject(args)];

      
      Scheduler.restartTimer();


      let reply;
      try {
        try {
          Scheduler.Debugging.messagesSent++;
          Scheduler.Debugging.latestSent = Scheduler.Debugging.latestSent.slice(0, 2);
          reply = yield this.worker.post(method, args, closure);
          Scheduler.Debugging.latestReceived = [Date.now(), summarizeObject(reply)];
          return reply;
        } finally {
          Scheduler.Debugging.messagesReceived++;
        }
      } catch (error) {
        Scheduler.Debugging.latestReceived = [Date.now(), error.message, error.fileName, error.lineNumber];
        throw error;
      } finally {
        if (firstLaunch) {
          Scheduler._updateTelemetry();
        }
        Scheduler.restartTimer();
      }
    }.bind(this)));
  },

  




  _updateTelemetry: function() {
    let worker = this.worker;
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

AsyncShutdown.webWorkersShutdown.addBlocker(
  "OS.File: flush pending requests, warn about unclosed files, shut down service.",
  Task.async(function*() {
    
    yield Barriers.shutdown.wait({crashAfterMS: null});

    
    yield Scheduler.kill({reset: false, shutdown: true});
  }),
  () => {
    let details = Barriers.getDetails();
    details.clients = Barriers.shutdown.state;
    return details;
  }
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
        () => Scheduler.kill({shutdown: false, reset: false})
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
  },

  



















  setPermissions: function setPermissions(options = {}) {
    return Scheduler.post("File_prototype_setPermissions",
                          [this._fdmsg, options]);
  }
};


if (SharedAll.Constants.Sys.Name != "Android" && SharedAll.Constants.Sys.Name != "Gonk") {
   











  File.prototype.setDates = function(accessDate, modificationDate) {
    return Scheduler.post("File_prototype_setDates",
      [this._fdmsg, accessDate, modificationDate], this);
  };
}









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






















File.setPermissions = function setPermissions(path, options = {}) {
  return Scheduler.post("setPermissions",
                        [Type.path.toMsg(path), options]);
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
    let {Deprecated} = Cu.import("resource://gre/modules/Deprecated.jsm", {});
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
  return Task.spawn(function*() {
    let resources = yield Scheduler.kill({shutdown: false, reset: true});
    if (resources && !resources.killed) {
        throw new Error("Could not reset worker, this would leak file descriptors: " + JSON.stringify(resources));
    }
  });
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


Object.defineProperty(OS.File, "queue", {
  get: function() {
    return Scheduler.queue;
  }
});




let Barriers = {
  profileBeforeChange: new AsyncShutdown.Barrier("OS.File: Waiting for clients before profile-before-shutdown"),
  shutdown: new AsyncShutdown.Barrier("OS.File: Waiting for clients before full shutdown"),
  


  getDetails: function() {
    let result = {
      launched: Scheduler.launched,
      shutdown: Scheduler.shutdown,
      worker: !!Scheduler._worker,
      pendingReset: !!Scheduler.resetTimer,
      latestSent: Scheduler.Debugging.latestSent,
      latestReceived: Scheduler.Debugging.latestReceived,
      messagesSent: Scheduler.Debugging.messagesSent,
      messagesReceived: Scheduler.Debugging.messagesReceived,
      messagesQueued: Scheduler.Debugging.messagesQueued,
      DEBUG: SharedAll.Config.DEBUG,
    };
    
    for (let key of ["latestSent", "latestReceived"]) {
      if (result[key] && typeof result[key][0] == "number") {
        result[key][0] = Date(result[key][0]);
      }
    }
    return result;
  }
};

File.profileBeforeChange = Barriers.profileBeforeChange.client;
File.shutdown = Barriers.shutdown.client;





AsyncShutdown.profileBeforeChange.addBlocker(
  "OS.File: flush I/O queued before profile-before-change",
  Task.async(function*() {
    
    yield Barriers.profileBeforeChange.wait({crashAfterMS: null});

    
    yield Scheduler.queue;
  }),
  () => {
    let details = Barriers.getDetails();
    details.clients = Barriers.profileBeforeChange.state;
    return details;
  }
);
