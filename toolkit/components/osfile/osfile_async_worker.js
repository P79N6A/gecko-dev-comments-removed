if (this.Components) {
  throw new Error("This worker can only be loaded from a worker thread");
}








(function(exports) {
  "use strict";

   try {
     importScripts("resource://gre/modules/osfile.jsm");

     let LOG = exports.OS.Shared.LOG.bind(exports.OS.Shared.LOG, "Agent");

     








     self.onmessage = function onmessage(msg) {
       let data = msg.data;
       LOG("Received message", data);
       let id = data.id;
       let result;
       let exn;
       try {
         let method = data.fun;
         LOG("Calling method", method);
         result = Agent[method].apply(Agent, data.args);
         LOG("Method", method, "succeeded");
       } catch (ex) {
         exn = ex;
         LOG("Error while calling agent method", exn, exn.stack);
       }
       
       
       
       
       if (!exn) {
         LOG("Sending positive reply", result, "id is", id);
         if (result instanceof Transfer) {
           
           self.postMessage({ok: result.data, id: id}, result.transfers);
         } else {
           self.postMessage({ok: result, id:id});
         }
       } else if (exn == StopIteration) {
         
         LOG("Sending back StopIteration");
         self.postMessage({StopIteration: true, id: id});
       } else if (exn instanceof exports.OS.File.Error) {
         LOG("Sending back OS.File error", exn, "id is", id);
         
         
         
         self.postMessage({fail: exports.OS.File.Error.toMsg(exn), id:id});
       } else {
         LOG("Sending back regular error", exn, exn.stack, "id is", id);
         
         
         
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

     











     let withFile = function withFile(id, f) {
       let file = OpenedFiles.get(id);
       if (file == null) {
         throw new Error("Could not find File");
       }
       return f.call(file);
     };

     let OpenedDirectoryIterators = new ResourceTracker();
     let withDir = function withDir(fd, f, ignoreAbsent) {
       let file = OpenedDirectoryIterators.get(fd);
       if (file == null) {
         if (!ignoreAbsent) {
           throw new Error("Could not find Directory");
         }
         return;
       }
       if (!(file instanceof File.DirectoryIterator)) {
         throw new Error("file is not a directory iterator " + file.__proto__.toSource());
       }
       return f.call(file);
     };

     let Type = exports.OS.Shared.Type;

     let File = exports.OS.File;

     









     let Transfer = function Transfer(data, transfers) {
       this.data = data;
       this.transfers = transfers;
     };

     






     let Agent = {
       
       SET_DEBUG: function SET_DEBUG (aDEBUG) {
         exports.OS.Shared.DEBUG = aDEBUG;
       },
       
       
       GET_DEBUG: function GET_DEBUG () {
         return exports.OS.Shared.DEBUG;
       },
       
       System_shutdown: function System_shutdown () {
         
         
         return {
           openedFiles: OpenedFiles.listOpenedResources(),
           openedDirectoryIterators:
             OpenedDirectoryIterators.listOpenedResources()
         };
       },
       
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
         let filePath = Type.path.fromMsg(path);
         let file = File.open(filePath, mode, options);
         return OpenedFiles.add(file, {
           
           
           path: filePath
         });
       },
       read: function read(path, bytes) {
         let data = File.read(Type.path.fromMsg(path), bytes);
         return new Transfer({buffer: data.buffer, byteOffset: data.byteOffset, byteLength: data.byteLength}, [data.buffer]);
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
       File_prototype_read: function read(fd, nbytes, options) {
         return withFile(fd,
           function do_read() {
             let data = this.read(nbytes, options);
             return new Transfer({buffer: data.buffer, byteOffset: data.byteOffset, byteLength: data.byteLength}, [data.buffer]);
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
  } catch(ex) {
    dump("WORKER ERROR DURING SETUP " + ex + "\n");
    dump("WORKER ERROR DETAIL " + ex.stack + "\n");
  }
})(this);
