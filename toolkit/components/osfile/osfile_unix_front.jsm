










{
  if (typeof Components != "undefined") {
    
    

    throw new Error("osfile_unix_front.jsm cannot be used from the main thread yet");
  }
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
         if (whence === undefined) {
           whence = Const.SEEK_START;
         }
         return throw_on_negative("setPosition",
           UnixFile.lseek(this.fd, pos, whence)
         );
       },

       




       stat: function stat() {
         throw_on_negative("stat", UnixFile.fstat(this.fd, gStatDataPtr));
         return new File.Info(gStatData);
       }
     };


     
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
     } 

     
     
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

       


       get isSymLink() {
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

     let gStatData = new OS.Shared.Type.stat.implementation();
     let gStatDataPtr = gStatData.address();
     let MODE_MASK = 4095 ;
     File.Info = function Info(stat) {
       this._st_mode = stat.st_mode;
       this._st_uid = stat.st_uid;
       this._st_gid = stat.st_gid;
       this._st_atime = stat.st_atime;
       this._st_mtime = stat.st_mtime;
       this._st_ctime = stat.st_ctime;
       this._st_size = stat.st_size;
     };
     File.Info.prototype = {
       


       get isDir() {
         return (this._st_mode & OS.Constants.libc.S_IFMT) == OS.Constants.libc.S_IFDIR;
       },
       


       get isSymLink() {
         return (this._st_mode & OS.Constants.libc.S_IFMT) == OS.Constants.libc.S_IFLNK;
       },
       







       get size() {
         return exports.OS.Shared.Type.size_t.importFromC(this._st_size);
       },
       




       get creationDate() {
         delete this.creationDate;
         let date = new Date(this._st_ctime * 1000);
         Object.defineProperty(this, "creationDate", { value: date });
         return date;
       },
       







       get lastAccessDate() {
         delete this.lastAccessDate;
         let date = new Date(this._st_atime * 1000);
         Object.defineProperty(this, "lastAccessDate", {value: date});
         return date;
       },
       


       get lastModificationDate() {
         delete this.lastModificationDate;
         let date = new Date(this._st_mtime * 1000);
         Object.defineProperty(this, "lastModificationDate", {value: date});
         return date;
       },
       


       get unixOwner() {
         return exports.OS.Shared.Type.uid_t.importFromC(this._st_uid);
       },
       


       get unixGroup() {
         return exports.OS.Shared.Type.gid_t.importFromC(this._st_gid);
       },
       


       get unixMode() {
         return exports.OS.Shared.Type.mode_t.importFromC(this._st_mode & MODE_MASK);
       }
     };

     




     File.Info.toMsg = function toMsg(stat) {
       if (!stat instanceof File.Info) {
         throw new TypeError("parameter of File.Info.toMsg must be a File.Info");
       }
       let serialized = {};
       for (let key in File.Info.prototype) {
         serialized[key] = stat[key];
       }
       return serialized;
     };

     











     File.stat = function stat(path, options) {
       options = options || noOptions;
       if (options.unixNoFollowingLinks) {
         throw_on_negative("stat", UnixFile.lstat(path, gStatDataPtr));
       } else {
         throw_on_negative("stat", UnixFile.stat(path, gStatDataPtr));
       }
       return new File.Info(gStatData);
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

     File.Unix = exports.OS.Unix.File;
     File.Error = exports.OS.Shared.Unix.Error;
     exports.OS.File = File;

     Object.defineProperty(File, "POS_START", { value: OS.Shared.POS_START });
     Object.defineProperty(File, "POS_CURRENT", { value: OS.Shared.POS_CURRENT });
     Object.defineProperty(File, "POS_END", { value: OS.Shared.POS_END });
   })(this);
}
