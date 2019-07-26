"use strict";




















this.EXPORTED_SYMBOLS = ["OS"];

Components.utils.import("resource://gre/modules/osfile/osfile_shared_allthreads.jsm", this);

let LOG = OS.Shared.LOG.bind(OS.Shared, "Controller");
let isTypedArray = OS.Shared.isTypedArray;


let DEBUG = OS.Shared.DEBUG;


let OSError;
if (OS.Constants.Win) {
  Components.utils.import("resource://gre/modules/osfile/osfile_win_allthreads.jsm", this);
  Components.utils.import("resource://gre/modules/osfile/ospath_win_back.jsm", this);
  OSError = OS.Shared.Win.Error;
} else if (OS.Constants.libc) {
  Components.utils.import("resource://gre/modules/osfile/osfile_unix_allthreads.jsm", this);
  Components.utils.import("resource://gre/modules/osfile/ospath_unix_back.jsm", this);
  OSError = OS.Shared.Unix.Error;
} else {
  throw new Error("I am neither under Windows nor under a Posix system");
}
let Type = OS.Shared.Type;


Components.utils.import("resource://gre/modules/commonjs/sdk/core/promise.js", this);


Components.utils.import("resource://gre/modules/osfile/_PromiseWorker.jsm", this);




if (!("profileDir" in OS.Constants.Path) || !("localProfileDir" in OS.Constants.Path)) {
  Components.utils.import("resource://gre/modules/Services.jsm", this);
  let observer = function observer() {
    Services.obs.removeObserver(observer, "profile-do-change");

    let profileDir = Services.dirsvc.get("ProfD", Components.interfaces.nsIFile).path;
    OS.Constants.Path.profileDir = profileDir;

    let localProfileDir = Services.dirsvc.get("ProfLD", Components.interfaces.nsIFile).path;
    OS.Constants.Path.localProfileDir = localProfileDir;
  };
  Services.obs.addObserver(observer, "profile-do-change", false);
}








let clone = function clone(object) {
  let result = {};
  for (let k in object) {
    result[k] = object[k];
  }
  return result;
};




const noOptions = {};


let worker = new PromiseWorker(
  "resource://gre/modules/osfile/osfile_async_worker.js",
  DEBUG?LOG:null);
let Scheduler = {
  post: function post(...args) {
    let promise = worker.post.apply(worker, args);
    return promise.then(
      null,
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


if (DEBUG === true) {
  Scheduler.post("SET_DEBUG", [DEBUG]);
}



Object.defineProperty(OS.Shared, "DEBUG", {
    configurable: true,
    get: function () {
        return DEBUG;
    },
    set: function (newVal) {
        Scheduler.post("SET_DEBUG", [newVal]);
        DEBUG = newVal;
    }
});









let File = function File(fdmsg) {
  
  
  this._fdmsg = fdmsg;
  this._closeResult = null;
  this._closed = null;
};


File.prototype = {
  








  close: function close() {
    if (this._fdmsg) {
      let msg = this._fdmsg;
      this._fdmsg = null;
      return this._closeResult =
        Scheduler.post("File_prototype_close", [msg], this);
    }
    return this._closeResult;
  },

  






  stat: function stat() {
    if (!this._fdmsg) {
      return Promise.reject(OSError.closed("accessing file"));
    }
    return Scheduler.post("File_prototype_stat", [this._fdmsg], this).then(
      File.Info.fromMsg
    );
  },

  











  readTo: function readTo(buffer, options) {
    
    
    
    if (isTypedArray(buffer) && (!options || !"bytes" in options)) {
      options = clone(options || noOptions);
      options.bytes = buffer.byteLength;
    }
    
    
    
    
    return Scheduler.post("File_prototype_readTo",
      [this._fdmsg,
       Type.void_t.out_ptr.toMsg(buffer),
       options],
       buffer);
  },
  

















  write: function write(buffer, options) {
    
    
    
    if (isTypedArray(buffer) && (!options || !"bytes" in options)) {
      options = clone(options || noOptions);
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











File.read = function read(path, bytes) {
  let promise = Scheduler.post("read",
    [Type.path.toMsg(path), bytes], path);
  return promise.then(
    function onSuccess(data) {
      return new Uint8Array(data.buffer, data.byteOffset, data.byteLength);
    });
};








File.exists = function exists(path) {
  return Scheduler.post("exists",
    [Type.path.toMsg(path)], path);
};
























File.writeAtomic = function writeAtomic(path, buffer, options) {
  
  options = clone(options || noOptions);
  
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
  return value;
};
if (OS.Constants.Win) {
  File.Info.prototype = Object.create(OS.Shared.Win.AbstractInfo.prototype);
} else if (OS.Constants.libc) {
  File.Info.prototype = Object.create(OS.Shared.Unix.AbstractInfo.prototype);
} else {
  throw new Error("I am neither under Windows nor under a Posix system");
}

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
      LOG("DirectoryIterator._next", "closed");
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
      return;
    }
    this._isClosed = true;
    let self = this;
    this._itmsg.then(
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
