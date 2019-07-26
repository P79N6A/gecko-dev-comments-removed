"use strict";




















let EXPORTED_SYMBOLS = ["OS"];

Components.utils.import("resource://gre/modules/osfile/osfile_shared_allthreads.jsm");

let LOG = OS.Shared.LOG.bind(OS.Shared, "Controller");




const DEBUG = true;


let OSError;
if (OS.Constants.Win) {
  Components.utils.import("resource://gre/modules/osfile/osfile_win_allthreads.jsm");
  Components.utils.import("resource://gre/modules/osfile/ospath_win_back.jsm");
  OSError = OS.Shared.Win.Error;
} else if (OS.Constants.libc) {
  Components.utils.import("resource://gre/modules/osfile/osfile_unix_allthreads.jsm");
  Components.utils.import("resource://gre/modules/osfile/ospath_unix_back.jsm");
  OSError = OS.Shared.Unix.Error;
} else {
  throw new Error("I am neither under Windows nor under a Posix system");
}
let Type = OS.Shared.Type;


Components.utils.import("resource://gre/modules/commonjs/promise/core.js");

const noOptions = {};







let Queue = function Queue() {
  
  
  
  this._pushing = null;

  
  
  
  this._popping = null;

  
  this._popindex = 0;
};
Queue.prototype = {
  


  push: function push(x) {
    if (!this._pushing) {
      this._pushing = [];
    }
    this._pushing.push({ value: x });
  },
  




  pop: function pop() {
    if (!this._popping) {
      if (!this._pushing) {
        throw new Error("Queue is empty");
      }
      this._popping = this._pushing;
      this._pushing = null;
      this._popindex = 0;
    }
    let result = this._popping[this._popindex];
    delete this._popping[this._popindex];
    ++this._popindex;
    if (this._popindex >= this._popping.length) {
      this._popping = null;
    }
    return result.value;
  }
};









let Scheduler = {
  


  get _worker() {
    delete this._worker;
    let worker = new ChromeWorker("osfile_async_worker.js");
    let self = this;
    Object.defineProperty(this, "_worker", {value:
      worker
    });

    











    worker.onerror = function onerror(error) {
      if (DEBUG) {
        LOG("Received uncaught error from worker", JSON.stringify(error.message), error.message);
      }
      error.preventDefault();
      let {deferred} = self._queue.pop();
      deferred.reject(error);
    };

    













    worker.onmessage = function onmessage(msg) {
      if (DEBUG) {
        LOG("Received message from worker", JSON.stringify(msg.data));
      }
      let handler = self._queue.pop();
      let deferred = handler.deferred;
      let data = msg.data;
      if (data.id != handler.id) {
        throw new Error("Internal error: expecting msg " + handler.id + ", " +
                        " got " + data.id + ": " + JSON.stringify(msg.data));
      }
      if ("ok" in data) {
        deferred.resolve(data.ok);
      } else if ("fail" in data) {
        let error;
        try {
          error = OS.File.Error.fromMsg(data.fail);
        } catch (x) {
          LOG("Cannot decode OS.File.Error", data.fail, data.id);
          deferred.reject(x);
          return;
        }
        deferred.reject(error);
      } else {
        throw new Error("Message does not respect protocol: " +
          data.toSource());
      }
    };
    return worker;
  },

  










  _queue: new Queue(),

  




  _id: 0,

  









  post: function post(fun, array, closure) {
    let deferred = Promise.defer();
    let id = ++this._id;
    let message = {fun: fun, args: array, id: id};
    if (DEBUG) {
      LOG("Posting message", JSON.stringify(message));
    }
    this._queue.push({deferred:deferred, closure: closure, id: id});
    this._worker.postMessage(message);
    if (DEBUG) {
      LOG("Message posted");
    }
    return deferred.promise;
  }
};









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
    
    
    
    if ("byteLength" in buffer && (!options || !"bytes" in options)) {
      options = Object.create(options || noOptions,
        {bytes: {value: buffer.byteLength, enumerable: true}});
    }
    
    
    
    
    return Scheduler.post("File_prototype_readTo",
      [this._fdmsg,
      Type.void_t.out_ptr.toMsg(buffer),
      options],
      buffer);
  },
  










  write: function write(buffer, options) {
    
    
    
    if ("byteLength" in buffer && (!options || !"bytes" in options)) {
      options = Object.create(options || noOptions,
        {bytes: {value: buffer.byteLength, enumerable: true}});
    }
    
    
    
    
    return Scheduler.post("File_prototype_write",
      [this._fdmsg,
      Type.void_t.in_ptr.toMsg(buffer),
      options],
      buffer);
  },

  










  read: function read(nbytes) {
    
    
    let self = this;
    let promise;
    if (nbytes != null) {
      promise = Promise.resolve(nbytes);
    } else {
      promise = this.stat();
      promise = promise.then(function withStat(stat) {
        return stat.size;
      });
    }
    let buffer;
    promise = promise.then(
      function withSize(size) {
        buffer = new ArrayBuffer(size);
        return self.readTo(buffer);
      }
    );
    promise = promise.then(
      function afterReadTo(bytes) {
        return {
          bytes: bytes,
          buffer: buffer
        };
      }
    );
    return promise;
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














File.makeDir = function makeDir(path, options) {
  return Scheduler.post("makeDir",
    [Type.path.toMsg(path), options], path);
};







File.Info = function Info(value) {
  return value;
};
File.Info.fromMsg = function fromMsg(value) {
  return new File.Info(value);
};


Object.defineProperty(File, "POS_START", {value: OS.Shared.POS_START});
Object.defineProperty(File, "POS_CURRENT", {value: OS.Shared.POS_CURRENT});
Object.defineProperty(File, "POS_END", {value: OS.Shared.POS_END});

OS.File = File;
OS.File.Error = OSError;

