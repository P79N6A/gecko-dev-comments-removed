










{
  if (typeof Components != "undefined") {
    
    

    throw new Error("osfile_unix_front.jsm cannot be used from the main thread yet");
  }
  importScripts("resource://gre/modules/osfile/osfile_shared.jsm");
  importScripts("resource://gre/modules/osfile/osfile_unix_back.jsm");
  importScripts("resource://gre/modules/osfile/ospath_unix_back.jsm");
  (function(exports) {
     "use strict";

     
     if (exports.OS.File) {
       return; 
     }
     exports.OS.Unix.File._init();
     let Const = exports.OS.Constants.libc;
     let UnixFile = exports.OS.Unix.File;
     let LOG = OS.Shared.LOG.bind(OS.Shared, "Unix front-end");

     








     let File = function File(fd) {
       this._fd = fd;
     };
     File.prototype = {
       



       get fd() {
         return this._fd;
       },

       
       
       _nofd: function nofd(operation) {
         operation = operation || "unknown operation";
         throw new File.Error(operation, Const.EBADF);
       },

       









       close: function close() {
         if (this._fd) {
           let fd = this._fd;
           this._fd = null;
           delete this.fd;
           Object.defineProperty(this, "fd", {get: File.prototype._nofd});
           
           if (UnixFile.close(fd) == -1) {
             this._closeResult = new File.Error("close", ctypes.errno);
           }
         }
         if (this._closeResult) {
           throw this._closeResult;
         }
         return;
       },
       _closeResult: null,

       














       read: function read(buffer, nbytes, options) {
         return throw_on_negative("read",
           UnixFile.read(this.fd, buffer, nbytes)
         );
       },

       












       write: function write(buffer, nbytes, options) {
         return throw_on_negative("write",
           UnixFile.write(this.fd, buffer, nbytes)
         );
       },

       


       getPosition: function getPosition(pos) {
         return this.setPosition(0, File.POS_CURRENT);
       },

       















       setPosition: function setPosition(pos, whence) {
         
         
         
         
         
         whence = (whence == undefined)?OS.Constants.libc.SEEK_SET:whence;
         return throw_on_negative("setPosition",
           UnixFile.lseek(this.fd, pos, whence)
         );
       }
     };

     






















     File.Error = function(operation, errno) {
       operation = operation || "unknown operation";
       OS.Shared.Error.call(this, operation);
       this.unixErrno = errno || ctypes.errno;
     };
     File.Error.prototype = new OS.Shared.Error();
     File.Error.prototype.toString = function toString() {
       return "Unix error " + this.unixErrno +
         " during operation " + this.operation +
         " (" + UnixFile.strerror(this.unixErrno).readString() + ")";
     };

     



     Object.defineProperty(File.Error.prototype, "becauseExists", {
       get: function becauseExists() {
         return this.unixErrno == OS.Constants.libc.EEXISTS;
       }
     });
     



     Object.defineProperty(File.Error.prototype, "becauseNoSuchFile", {
       get: function becauseNoSuchFile() {
         return this.unixErrno == OS.Constants.libc.ENOENT;
       }
     });

     
     const noOptions = {};

     
     const DEFAULT_UNIX_MODE = 384;

     










































     File.open = function Unix_open(path, mode, options) {
       options = options || noOptions;
       let omode = options.unixMode || DEFAULT_UNIX_MODE;
       let flags;
       if (options.unixFlags) {
         flags = options.unixFlags;
       } else {
         mode = OS.Shared._aux.normalizeOpenMode(mode);
         
         if (!mode.write) {
           flags = Const.O_RDONLY;
         } else if (mode.read) {
           flags = Const.O_RDWR;
         } else {
           flags = Const.O_WRONLY;
         }
         
         if (mode.trunc) {
           if (mode.existing) {
             flags |= Const.O_TRUNC;
           } else {
             flags |= Const.O_CREAT | Const.O_TRUNC;
           }
         } else if (mode.create) {
           flags |= Const.O_CREAT | Const.O_EXCL;
         } else if (mode.read && !mode.write) {
           
         } else  {
           if (mode.existing) {
             flags |= Const.O_APPEND;
           } else {
             flags |= Const.O_APPEND | Const.O_CREAT;
           }
         }
       }
       return error_or_file(UnixFile.open(path, flags, omode));
     };

     




     File.remove = function remove(path) {
       throw_on_negative("remove",
         UnixFile.unlink(path)
       );
     };

     






















     File.copy = null;

     






















     File.move = null;

     if (UnixFile.copyfile) {
       
       
       
       File.copy = function copyfile(sourcePath, destPath, options) {
         options = options || noOptions;
         let flags = Const.COPYFILE_DATA;
         if (options.noOverwrite) {
           flags |= Const.COPYFILE_EXCL;
         }
         throw_on_negative("copy",
           UnixFile.copyfile(sourcePath, destPath, null, flags)
         );
       };

       
       
       
       File.move = function movefile(sourcePath, destPath, options) {
         
         
         options = options || noOptions;
         let flags = Const.COPYFILE_DATA | Const.COPYFILE_MOVE;
         if (options.noOverwrite) {
           flags |= Const.COPYFILE_EXCL;
         }
         throw_on_negative("move",
           UnixFile.copyfile(sourcePath, destPath, null, flags)
         );
       };
     } else {
       
       
       

       

















       let pump;

       
       let pump_buffer = null;

       
       let pump_userland = function pump_userland(source, dest, options) {
         let options = options || noOptions;
         let bufSize = options.bufSize || 4096;
         let nbytes = options.nbytes || Infinity;
         if (!pump_buffer || pump_buffer.length < bufSize) {
           pump_buffer = new (ctypes.ArrayType(ctypes.char))(bufSize);
         }
         let read = source.read.bind(source);
         let write = dest.write.bind(dest);
         
         let total_read = 0;
         while (true) {
           let chunk_size = Math.min(nbytes, bufSize);
           let bytes_just_read = read(pump_buffer, bufSize);
           if (bytes_just_read == 0) {
             return total_read;
           }
           total_read += bytes_just_read;
           let bytes_written = 0;
           do {
             bytes_written += write(
               pump_buffer.addressOfElement(bytes_written),
               bytes_just_read - bytes_written
             );
           } while (bytes_written < bytes_just_read);
           nbytes -= bytes_written;
           if (nbytes <= 0) {
             return total_read;
           }
         }
       };

       
       if (UnixFile.splice) {
         const BUFSIZE = 1 << 17;

         
         pump = function pump_splice(source, dest, options) {
           let options = options || noOptions;
           let nbytes = options.nbytes || Infinity;
           let pipe = [];
           throw_on_negative("pump", UnixFile.pipe(pipe));
           let pipe_read = pipe[0];
           let pipe_write = pipe[1];
           let source_fd = source.fd;
           let dest_fd = dest.fd;
           let total_read = 0;
           let total_written = 0;
           try {
             while (true) {
               let chunk_size = Math.min(nbytes, BUFSIZE);
               let bytes_read = throw_on_negative("pump",
                 UnixFile.splice(source_fd, null,
                 pipe_write, null, chunk_size, 0)
               );
               if (!bytes_read) {
                 break;
               }
               total_read += bytes_read;
               let bytes_written = throw_on_negative(
                 "pump",
                 UnixFile.splice(pipe_read, null,
                   dest_fd, null, bytes_read,
                     (bytes_read == chunk_size)?Const.SPLICE_F_MORE:0
               ));
               if (!bytes_written) {
                 
                 throw new Error("Internal error: pipe disconnected");
               }
               total_written += bytes_written;
               nbytes -= bytes_read;
               if (!nbytes) {
                 break;
               }
             }
             return total_written;
           } catch (x) {
             if (x.unixErrno == Const.EINVAL) {
               
               
               if (total_read) {
                 source.setPosition(-total_read, OS.File.POS_CURRENT);
               }
               if (total_written) {
                 dest.setPosition(-total_written, OS.File.POS_CURRENT);
               }
               return pump_userland(source, dest, options);
             }
             throw x;
           } finally {
             pipe_read.dispose();
             pipe_write.dispose();
           }
         };
     } else {
       
       pump = pump_userland;
     }

     
     
     
     File.copy = function copy(sourcePath, destPath, options) {
       options = options || noOptions;
       let source, dest;
       let result;
       try {
         source = File.open(sourcePath);
         if (options.noOverwrite) {
           dest = File.open(destPath, {create:true});
         } else {
           dest = File.open(destPath, {write:true});
         }
         result = pump(source, dest, options);
       } catch (x) {
         if (dest) {
           dest.close();
         }
         if (source) {
           source.close();
         }
         throw x;
       }
     };

     
     
     File.move = function move(sourcePath, destPath, options) {
       
       
       
       

       options = options || noOptions;

       
       if (options.noOverwrite) {
         let fd = UnixFile.open(destPath, Const.O_RDONLY, 0);
         if (fd != -1) {
           fd.dispose();
           
           throw new File.Error("move", Const.EEXIST);
         } else if (ctypes.errno == Const.EACCESS) {
           
           throw new File.Error("move", Const.EEXIST);
         }
       }

       
       let result = UnixFile.rename(sourcePath, destPath);
       if (result != -1)
         return;

       
       
       if (ctypes.errno != Const.EXDEV) {
         throw new File.Error();
       }

         
         File.copy(sourcePath, destPath, options);
         
         File.remove(sourcePath);
       };

     } 

     











     File.DirectoryIterator = function DirectoryIterator(path, options) {
       let dir = throw_on_null("DirectoryIterator", UnixFile.opendir(path));
       this._dir = dir;
       this._path = path;
     };
     File.DirectoryIterator.prototype = {
       __iterator__: function __iterator__() {
         return this;
       },
       









       next: function next() {
         if (!this._dir) {
           throw StopIteration;
         }
         for (let entry = UnixFile.readdir(this._dir);
              entry != null && !entry.isNull();
              entry = UnixFile.readdir(this._dir)) {
           let contents = entry.contents;
           if (contents.d_type == OS.Constants.libc.DT_DIR) {
             let name = contents.d_name.readString();
             if (name == "." || name == "..") {
               continue;
             }
           }
           return new File.DirectoryIterator.Entry(contents, this._path);
         }
         this.close();
         throw StopIteration;
       },

       



       close: function close() {
         if (!this._dir) return;
         UnixFile.closedir(this._dir);
         this._dir = null;
       }
     };

     


     File.DirectoryIterator.Entry = function Entry(unix_entry, parent) {
       
       
       this._d_type = unix_entry.d_type;
       this._name = unix_entry.d_name.readString();
       this._parent = parent;
     };
     File.DirectoryIterator.Entry.prototype = {
       


       get isDir() {
         return this._d_type == OS.Constants.libc.DT_DIR;
       },

       


       get isLink() {
         return this._d_type == OS.Constants.libc.DT_LNK;
       },

       



       get name() {
         return this._name;
       },

       


       get path() {
         delete this.path;
         let path = OS.Unix.Path.join(this._parent, this.name);
         Object.defineProperty(this, "path", {value: path});
         return path;
       }
     };


     


     Object.defineProperty(File, "curDir", {
         set: function(path) {
           throw_on_negative("curDir",
             UnixFile.chdir(path)
           );
         },
         get: function() {
           let path = UnixFile.get_current_dir_name?UnixFile.get_current_dir_name():
             UnixFile.getwd_auto(null);
           throw_on_null("curDir",path);
           return path.readString();
         }
       }
     );

     

     


     function error_or_file(maybe) {
       if (maybe == -1) {
         throw new File.Error("open");
       }
       return new File(maybe);
     }

     








     function throw_on_negative(operation, result) {
       if (result < 0) {
         throw new File.Error(operation);
       }
       return result;
     }

     function throw_on_null(operation, result) {
       if (result == null || (result.isNull && result.isNull())) {
         throw new File.Error(operation);
       }
       return result;
     }

     File.POS_START = exports.OS.Constants.libc.SEEK_SET;
     File.POS_CURRENT = exports.OS.Constants.libc.SEEK_CUR;
     File.POS_END = exports.OS.Constants.libc.SEEK_END;

     File.Unix = exports.OS.Unix.File;
     exports.OS.File = File;
   })(this);
}
