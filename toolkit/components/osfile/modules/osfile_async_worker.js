if (this.Components) {
  throw new Error("This worker can only be loaded from a worker thread");
}









const EXCEPTION_NAMES = {
  EvalError: "EvalError",
  InternalError: "InternalError",
  RangeError: "RangeError",
  ReferenceError: "ReferenceError",
  SyntaxError: "SyntaxError",
  TypeError: "TypeError",
  URIError: "URIError"
};

(function(exports) {
  "use strict";

  
  
  
  let timeStamps = {
    entered: Date.now(),
    loaded: null
  };

  importScripts("resource://gre/modules/osfile.jsm");

  let SharedAll = require("resource://gre/modules/osfile/osfile_shared_allthreads.jsm");
  let LOG = SharedAll.LOG.bind(SharedAll, "Agent");

  
  
  function post(message, ...transfers) {
    if (timeStamps) {
      message.timeStamps = timeStamps;
      timeStamps = null;
    }
    self.postMessage(message, ...transfers);
  }

 








  self.onmessage = function onmessage(msg) {
   let data = msg.data;
   LOG("Received message", data);
   let id = data.id;

   let start;
   let options;
   if (data.args) {
     options = data.args[data.args.length - 1];
   }
   
   
   if (options && typeof options === "object" && "outExecutionDuration" in options) {
     start = Date.now();
   }

   let result;
   let exn;
   let durationMs;
   try {
     let method = data.fun;
     LOG("Calling method", method);
     result = Agent[method].apply(Agent, data.args);
     LOG("Method", method, "succeeded");
   } catch (ex) {
     exn = ex;
     LOG("Error while calling agent method", exn, exn.stack || "");
   }

   if (start) {
     
     durationMs = Date.now() - start;
     LOG("Method took", durationMs, "ms");
   }

   
   
   
   
   if (!exn) {
     LOG("Sending positive reply", result, "id is", id);
     if (result instanceof Meta) {
       if ("transfers" in result.meta) {
         
         post({ok: result.data, id: id, durationMs: durationMs},
           result.meta.transfers);
       } else {
         post({ok: result.data, id:id, durationMs: durationMs});
       }
       if (result.meta.shutdown || false) {
         
         self.close();
       }
     } else {
       post({ok: result, id:id, durationMs: durationMs});
     }
   } else if (exn == StopIteration) {
     
     LOG("Sending back StopIteration");
     post({StopIteration: true, id: id, durationMs: durationMs});
   } else if (exn instanceof exports.OS.File.Error) {
     LOG("Sending back OS.File error", exn, "id is", id);
     
     
     
     post({fail: exports.OS.File.Error.toMsg(exn), id:id, durationMs: durationMs});
   } else if (exn.constructor.name in EXCEPTION_NAMES) {
     LOG("Sending back exception", exn.constructor.name);
     post({fail: {exn: exn.constructor.name, message: exn.message,
                  fileName: exn.moduleName || exn.fileName, lineNumber: exn.lineNumber},
           id: id, durationMs: durationMs});
   } else {
     
     
     
     LOG("Sending back regular error", exn, exn.moduleStack || exn.stack, "id is", id);

     throw exn;
   }
  };

 


  let ResourceTracker = function ResourceTracker() {
   
   this._idgen = 0;
   
   this._map = new Map();
  };
  ResourceTracker.prototype = {
   


   get: function(id) {
     let result = this._map.get(id);
     if (result == null) {
       return result;
     }
     return result.resource;
   },
   


   remove: function(id) {
     if (!this._map.has(id)) {
       throw new Error("Cannot find resource id " + id);
     }
     this._map.delete(id);
   },
   








   add: function(resource, info) {
     let id = this._idgen++;
     this._map.set(id, {resource:resource, info:info});
     return id;
   },
   



   listOpenedResources: function listOpenedResources() {
     return [resource.info.path for ([id, resource] of this._map)];
   }
  };

 


  let OpenedFiles = new ResourceTracker();

 











  let withFile = function withFile(id, f, ignoreAbsent) {
   let file = OpenedFiles.get(id);
   if (file == null) {
     if (!ignoreAbsent) {
       throw OS.File.Error.closed("accessing file");
     }
     return undefined;
   }
   return f.call(file);
  };

  let OpenedDirectoryIterators = new ResourceTracker();
  let withDir = function withDir(fd, f, ignoreAbsent) {
   let file = OpenedDirectoryIterators.get(fd);
   if (file == null) {
     if (!ignoreAbsent) {
       throw OS.File.Error.closed("accessing directory");
     }
     return undefined;
   }
   if (!(file instanceof File.DirectoryIterator)) {
     throw new Error("file is not a directory iterator " + file.__proto__.toSource());
   }
   return f.call(file);
  };

  let Type = exports.OS.Shared.Type;

  let File = exports.OS.File;

  














  let Meta = function Meta(data, meta) {
    this.data = data;
    this.meta = meta;
  };

 






  let Agent = {
   
   SET_DEBUG: function(aDEBUG) {
     SharedAll.Config.DEBUG = aDEBUG;
   },
   
   
   GET_DEBUG: function() {
     return SharedAll.Config.DEBUG;
   },
   Meta_getUnclosedResources: function() {
     
     
     return {
       openedFiles: OpenedFiles.listOpenedResources(),
       openedDirectoryIterators: OpenedDirectoryIterators.listOpenedResources()
     };
   },
   Meta_reset: function() {
     
     
     
     
     let openedFiles = OpenedFiles.listOpenedResources();
     let openedDirectoryIterators =
       OpenedDirectoryIterators.listOpenedResources();
     let canShutdown = openedFiles.length == 0
                         && openedDirectoryIterators.length == 0;
     if (canShutdown) {
       
       return new Meta(null, {shutdown: true});
     } else {
       
       return {
         openedFiles: openedFiles,
         openedDirectoryIterators: openedDirectoryIterators
       };
     }
   },
   
   stat: function stat(path, options) {
     return exports.OS.File.Info.toMsg(
       exports.OS.File.stat(Type.path.fromMsg(path), options));
   },
   setDates: function setDates(path, accessDate, modificationDate) {
     return exports.OS.File.setDates(Type.path.fromMsg(path), accessDate,
                                     modificationDate);
   },
   getCurrentDirectory: function getCurrentDirectory() {
     return exports.OS.Shared.Type.path.toMsg(File.getCurrentDirectory());
   },
   setCurrentDirectory: function setCurrentDirectory(path) {
     File.setCurrentDirectory(exports.OS.Shared.Type.path.fromMsg(path));
   },
   copy: function copy(sourcePath, destPath, options) {
     return File.copy(Type.path.fromMsg(sourcePath),
       Type.path.fromMsg(destPath), options);
   },
   move: function move(sourcePath, destPath, options) {
     return File.move(Type.path.fromMsg(sourcePath),
       Type.path.fromMsg(destPath), options);
   },
   getAvailableFreeSpace: function getAvailableFreeSpace(sourcePath) {
     return Type.uint64_t.toMsg(
       File.getAvailableFreeSpace(Type.path.fromMsg(sourcePath)));
   },
   makeDir: function makeDir(path, options) {
     return File.makeDir(Type.path.fromMsg(path), options);
   },
   removeEmptyDir: function removeEmptyDir(path, options) {
     return File.removeEmptyDir(Type.path.fromMsg(path), options);
   },
   remove: function remove(path) {
     return File.remove(Type.path.fromMsg(path));
   },
   open: function open(path, mode, options) {
     let filePath = Type.path.fromMsg(path);
     let file = File.open(filePath, mode, options);
     return OpenedFiles.add(file, {
       
       
       path: filePath
     });
   },
   openUnique: function openUnique(path, options) {
     let filePath = Type.path.fromMsg(path);
     let openedFile = OS.Shared.AbstractFile.openUnique(filePath, options);
     let resourceId = OpenedFiles.add(openedFile.file, {
       
       
       path: openedFile.path
     });

     return {
       path: openedFile.path,
       file: resourceId
     };
   },
   read: function read(path, bytes, options) {
     let data = File.read(Type.path.fromMsg(path), bytes, options);
     if (typeof data == "string") {
       return data;
     }
     return new Meta({
         buffer: data.buffer,
         byteOffset: data.byteOffset,
         byteLength: data.byteLength
     }, {
       transfers: [data.buffer]
     });
   },
   exists: function exists(path) {
     return File.exists(Type.path.fromMsg(path));
   },
   writeAtomic: function writeAtomic(path, buffer, options) {
     if (options.tmpPath) {
       options.tmpPath = Type.path.fromMsg(options.tmpPath);
     }
     return File.writeAtomic(Type.path.fromMsg(path),
                             Type.voidptr_t.fromMsg(buffer),
                             options
                            );
   },
   removeDir: function(path, options) {
     return File.removeDir(Type.path.fromMsg(path), options);
   },
   new_DirectoryIterator: function new_DirectoryIterator(path, options) {
     let directoryPath = Type.path.fromMsg(path);
     let iterator = new File.DirectoryIterator(directoryPath, options);
     return OpenedDirectoryIterators.add(iterator, {
       
       
       path: directoryPath
     });
   },
   
   File_prototype_close: function close(fd) {
     return withFile(fd,
       function do_close() {
         try {
           return this.close();
         } finally {
           OpenedFiles.remove(fd);
         }
     });
   },
   File_prototype_stat: function stat(fd) {
     return withFile(fd,
       function do_stat() {
         return exports.OS.File.Info.toMsg(this.stat());
       });
   },
   File_prototype_setDates: function setDates(fd, accessTime, modificationTime) {
     return withFile(fd,
       function do_setDates() {
         return this.setDates(accessTime, modificationTime);
       });
   },
   File_prototype_read: function read(fd, nbytes, options) {
     return withFile(fd,
       function do_read() {
         let data = this.read(nbytes, options);
         return new Meta({
             buffer: data.buffer,
             byteOffset: data.byteOffset,
             byteLength: data.byteLength
         }, {
           transfers: [data.buffer]
         });
       }
     );
   },
   File_prototype_readTo: function readTo(fd, buffer, options) {
     return withFile(fd,
       function do_readTo() {
         return this.readTo(exports.OS.Shared.Type.voidptr_t.fromMsg(buffer),
         options);
       });
   },
   File_prototype_write: function write(fd, buffer, options) {
     return withFile(fd,
       function do_write() {
         return this.write(exports.OS.Shared.Type.voidptr_t.fromMsg(buffer),
         options);
       });
   },
   File_prototype_setPosition: function setPosition(fd, pos, whence) {
     return withFile(fd,
       function do_setPosition() {
         return this.setPosition(pos, whence);
       });
   },
   File_prototype_getPosition: function getPosition(fd) {
     return withFile(fd,
       function do_getPosition() {
         return this.getPosition();
       });
   },
   File_prototype_flush: function flush(fd) {
     return withFile(fd,
       function do_flush() {
         return this.flush();
       });
   },
   
   DirectoryIterator_prototype_next: function next(dir) {
     return withDir(dir,
       function do_next() {
         try {
           return File.DirectoryIterator.Entry.toMsg(this.next());
         } catch (x) {
           if (x == StopIteration) {
             OpenedDirectoryIterators.remove(dir);
           }
           throw x;
         }
       }, false);
   },
   DirectoryIterator_prototype_nextBatch: function nextBatch(dir, size) {
     return withDir(dir,
       function do_nextBatch() {
         let result;
         try {
           result = this.nextBatch(size);
         } catch (x) {
           OpenedDirectoryIterators.remove(dir);
           throw x;
         }
         return result.map(File.DirectoryIterator.Entry.toMsg);
       }, false);
   },
   DirectoryIterator_prototype_close: function close(dir) {
     return withDir(dir,
       function do_close() {
         this.close();
         OpenedDirectoryIterators.remove(dir);
       }, true);
   },
   DirectoryIterator_prototype_exists: function exists(dir) {
     return withDir(dir,
       function do_exists() {
         return this.exists();
       });
   }
  };
  if (!SharedAll.Constants.Win) {
    Agent.unixSymLink = function unixSymLink(sourcePath, destPath) {
      return File.unixSymLink(Type.path.fromMsg(sourcePath),
        Type.path.fromMsg(destPath));
    };
  }

  timeStamps.loaded = Date.now();
})(this);
