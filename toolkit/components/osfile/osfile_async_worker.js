if (this.Components) {
  throw new Error("This worker can only be loaded from a worker thread");
}








(function(exports) {
  "use strict";

  
  
  
  const DEBUG = false;

   try {
     importScripts("resource://gre/modules/osfile.jsm");

     let LOG = exports.OS.Shared.LOG.bind(exports.OS.Shared.LOG, "Agent");

     








     self.onmessage = function onmessage(msg) {
       let data = msg.data;
       if (DEBUG) {
         LOG("Received message", JSON.stringify(data));
       }
       let id = data.id;
       let result;
       let exn;
       try {
         let method = data.fun;
         if (DEBUG) {
           LOG("Calling method", method);
         }
         result = Agent[method].apply(Agent, data.args);
         if (DEBUG) {
           LOG("Method", method, "succeeded");
         }
       } catch (ex) {
         exn = ex;
         if (DEBUG) {
           LOG("Error while calling agent method", exn, exn.stack);
         }
       }
       
       
       
       
       if (!exn) {
         if (DEBUG) {
           LOG("Sending positive reply", JSON.stringify(result), "id is", id);
         }
         self.postMessage({ok: result, id:id});
       } else if (exn instanceof exports.OS.File.Error) {
         if (DEBUG) {
           LOG("Sending back OS.File error", exn, "id is", id);
         }
         
         
         
         self.postMessage({fail: exports.OS.File.Error.toMsg(exn), id:id});
       } else {
         if (DEBUG) {
           LOG("Sending back regular error", exn, exn.stack, "id is", id);
         }
         
         
         
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
       }
     };

     


     let OpenedFiles = new ResourceTracker();

     










     let withFile = function withFile(id, f) {
       let file = OpenedFiles.get(id);
       if (file == null) {
         throw new Error("Could not find File");
       }
       return f.call(file);
     };

     let OpenedDirectoryIterators = new ResourceTracker();
     let withDir = function withDir(fd, f) {
       let file = OpenedDirectoryIterators.get(fd);
       if (file == null) {
         throw new Error("Could not find Directory");
       }
       if (!(file instanceof File.DirectoryIterator)) {
         throw new Error("file is not a directory iterator " + file.__proto__.toSource());
       }
       return f.call(file);
     };

     let Type = exports.OS.Shared.Type;

     let File = exports.OS.File;

     






     let Agent = {
       
       stat: function stat(path) {
         return exports.OS.File.Info.toMsg(
           exports.OS.File.stat(Type.path.fromMsg(path)));
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
         let file = File.open(Type.path.fromMsg(path), mode, options);
         return OpenedFiles.add(file);
       },
       read: function read(path, bytes) {
         return File.read(Type.path.fromMsg(path), bytes);
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
       new_DirectoryIterator: function new_DirectoryIterator(path, options) {
         let iterator = new File.DirectoryIterator(Type.path.fromMsg(path), options);
         return OpenedDirectoryIterators.add(iterator);
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
           });
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
           });
       },
       DirectoryIterator_prototype_close: function close(dir) {
         return withDir(dir,
           function do_close() {
             this.close();
             OpenedDirectoryIterators.remove(dir);
           });
       }
     };
  } catch(ex) {
    dump("WORKER ERROR DURING SETUP " + ex + "\n");
    dump("WORKER ERROR DETAIL " + ex.stack + "\n");
  }
})(this);