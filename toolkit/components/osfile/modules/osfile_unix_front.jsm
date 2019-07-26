










{
  if (typeof Components != "undefined") {
    
    

    throw new Error("osfile_unix_front.jsm cannot be used from the main thread yet");
  }
  (function(exports) {
     "use strict";

     
     if (exports.OS && exports.OS.File) {
       return; 
     }

     let SharedAll = require("resource://gre/modules/osfile/osfile_shared_allthreads.jsm");
     let Path = require("resource://gre/modules/osfile/ospath.jsm");
     let SysAll = require("resource://gre/modules/osfile/osfile_unix_allthreads.jsm");
     exports.OS.Unix.File._init();
     let LOG = SharedAll.LOG.bind(SharedAll, "Unix front-end");
     let Const = SharedAll.Constants.libc;
     let UnixFile = exports.OS.Unix.File;
     let Type = UnixFile.Type;

     








     let File = function File(fd) {
       exports.OS.Shared.AbstractFile.call(this, fd);
       this._closeResult = null;
     };
     File.prototype = Object.create(exports.OS.Shared.AbstractFile.prototype);

     









     File.prototype.close = function close() {
       if (this._fd) {
         let fd = this._fd;
         this._fd = null;
        
         
         
         let result = UnixFile._close(fd);
         if (typeof fd == "object" && "forget" in fd) {
           fd.forget();
         }
         if (result == -1) {
           this._closeResult = new File.Error("close");
         }
       }
       if (this._closeResult) {
         throw this._closeResult;
       }
       return;
     };

     














     File.prototype._read = function _read(buffer, nbytes, options = {}) {
      
      
       if (typeof(UnixFile.posix_fadvise) === 'function' &&
           (options.sequential || !("sequential" in options))) {
         UnixFile.posix_fadvise(this.fd, 0, nbytes,
          OS.Constants.libc.POSIX_FADV_SEQUENTIAL);
       }
       return throw_on_negative("read",
         UnixFile.read(this.fd, buffer, nbytes)
       );
     };

     












     File.prototype._write = function _write(buffer, nbytes, options = {}) {
       return throw_on_negative("write",
         UnixFile.write(this.fd, buffer, nbytes)
       );
     };

     


     File.prototype.getPosition = function getPosition(pos) {
         return this.setPosition(0, File.POS_CURRENT);
     };

     















     File.prototype.setPosition = function setPosition(pos, whence) {
       if (whence === undefined) {
         whence = Const.SEEK_SET;
       }
       return throw_on_negative("setPosition",
         UnixFile.lseek(this.fd, pos, whence)
       );
     };

     




     File.prototype.stat = function stat() {
       throw_on_negative("stat", UnixFile.fstat(this.fd, gStatDataPtr));
         return new File.Info(gStatData);
     };

     





     File.prototype.flush = function flush() {
       throw_on_negative("flush", UnixFile.fsync(this.fd));
     };

     
     const DEFAULT_UNIX_MODE = 384;

     













































     File.open = function Unix_open(path, mode, options = {}) {
       let omode = options.unixMode !== undefined ?
                     options.unixMode : DEFAULT_UNIX_MODE;
       let flags;
       if (options.unixFlags !== undefined) {
         flags = options.unixFlags;
       } else {
         mode = OS.Shared.AbstractFile.normalizeOpenMode(mode);
         
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
           
         } else if (!mode.existing) {
           flags |= Const.O_CREAT;
         }
         if (mode.append) {
           flags |= Const.O_APPEND;
         }
       }
       return error_or_file(UnixFile.open(path, flags, omode));
     };

     






     File.exists = function Unix_exists(path) {
       if (UnixFile.access(path, Const.F_OK) == -1) {
         return false;
       } else {
         return true;
       }
     };

     









     File.remove = function remove(path, options = {}) {
       let result = UnixFile.unlink(path);
       if (result == -1) {
         if ((!("ignoreAbsent" in options) || options.ignoreAbsent) &&
             ctypes.errno == Const.ENOENT) {
           return;
         }
         throw new File.Error("remove");
       }
     };

     







     File.removeEmptyDir = function removeEmptyDir(path, options = {}) {
       let result = UnixFile.rmdir(path);
       if (result == -1) {
         if (options.ignoreAbsent && ctypes.errno == Const.ENOENT) {
           return;
         }
         throw new File.Error("removeEmptyDir");
       }
     };

     


     const DEFAULT_UNIX_MODE_DIR = Const.S_IRWXU;

     













     File.makeDir = function makeDir(path, options = {}) {
       let omode = options.unixMode !== undefined ? options.unixMode : DEFAULT_UNIX_MODE_DIR;
       let result = UnixFile.mkdir(path, omode);
       if (result != -1 ||
           options.ignoreExisting && ctypes.errno == Const.EEXIST) {
        return;
       }
       throw new File.Error("makeDir");
     };

     






















     File.copy = null;

     

























     File.move = null;

     if (UnixFile.copyfile) {
       
       
       
       File.copy = function copyfile(sourcePath, destPath, options = {}) {
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

       
       let pump_userland = function pump_userland(source, dest, options = {}) {
         let bufSize = options.bufSize > 0 ? options.bufSize : 4096;
         let nbytes = options.nbytes > 0 ? options.nbytes : Infinity;
         if (!pump_buffer || pump_buffer.length < bufSize) {
           pump_buffer = new (ctypes.ArrayType(ctypes.char))(bufSize);
         }
         let read = source._read.bind(source);
         let write = dest._write.bind(dest);
         
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

         
         pump = function pump_splice(source, dest, options = {}) {
           let nbytes = options.nbytes > 0 ? options.nbytes : Infinity;
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
                 source.setPosition(-total_read, File.POS_CURRENT);
               }
               if (total_written) {
                 dest.setPosition(-total_written, File.POS_CURRENT);
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

       
       
       
       File.copy = function copy(sourcePath, destPath, options = {}) {
         let source, dest;
         let result;
         try {
           source = File.open(sourcePath);
           
           
           if (options.noOverwrite) {
             dest = File.open(destPath, {create:true, append:false});
           } else {
             dest = File.open(destPath, {trunc:true, append:false});
           }
           if (options.unixUserland) {
             result = pump_userland(source, dest, options);
           } else {
             result = pump(source, dest, options);
           }
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

     
     
     File.move = function move(sourcePath, destPath, options = {}) {
       
       
       
       

       
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

       
       
       
       
       if (ctypes.errno != Const.EXDEV || options.noCopy) {
         throw new File.Error("move");
       }

       
       File.copy(sourcePath, destPath, options);
       
       File.remove(sourcePath);
     };

     











     File.DirectoryIterator = function DirectoryIterator(path, options) {
       exports.OS.Shared.AbstractFile.AbstractIterator.call(this);
       this._path = path;
       this._dir = UnixFile.opendir(this._path);
       if (this._dir == null) {
         let error = ctypes.errno;
         if (error != Const.ENOENT) {
           throw new File.Error("DirectoryIterator", error);
         }
         this._exists = false;
         this._closed = true;
       } else {
         this._exists = true;
         this._closed = false;
       }
     };
     File.DirectoryIterator.prototype = Object.create(exports.OS.Shared.AbstractFile.AbstractIterator.prototype);

     









     File.DirectoryIterator.prototype.next = function next() {
       if (!this._exists) {
         throw File.Error.noSuchFile("DirectoryIterator.prototype.next");
       }
       if (this._closed) {
         throw StopIteration;
       }
       for (let entry = UnixFile.readdir(this._dir);
            entry != null && !entry.isNull();
            entry = UnixFile.readdir(this._dir)) {
         let contents = entry.contents;
         let name = contents.d_name.readString();
         if (name == "." || name == "..") {
           continue;
         }

         let isDir, isSymLink;
         if (!("d_type" in contents)) {
           
           let path = Path.join(this._path, name);
           throw_on_negative("lstat", UnixFile.lstat(path, gStatDataPtr));
           isDir = (gStatData.st_mode & Const.S_IFMT) == Const.S_IFDIR;
           isSymLink = (gStatData.st_mode & Const.S_IFMT) == Const.S_IFLNK;
         } else {
           isDir = contents.d_type == Const.DT_DIR;
           isSymLink = contents.d_type == Const.DT_LNK;
         }

         return new File.DirectoryIterator.Entry(isDir, isSymLink, name, this._path);
       }
       this.close();
       throw StopIteration;
     };

     



     File.DirectoryIterator.prototype.close = function close() {
       if (this._closed) return;
       this._closed = true;
       UnixFile.closedir(this._dir);
       this._dir = null;
     };

    




     File.DirectoryIterator.prototype.exists = function exists() {
       return this._exists;
     };

     


     File.DirectoryIterator.prototype.unixAsFile = function unixAsFile() {
       if (!this._dir) throw File.Error.closed();
       return error_or_file(UnixFile.dirfd(this._dir));
     };

     


     File.DirectoryIterator.Entry = function Entry(isDir, isSymLink, name, parent) {
       
       
       this._parent = parent;
       let path = Path.join(this._parent, name);

       SysAll.AbstractEntry.call(this, isDir, isSymLink, name, path);
     };
     File.DirectoryIterator.Entry.prototype = Object.create(SysAll.AbstractEntry.prototype);

     






     File.DirectoryIterator.Entry.toMsg = function toMsg(value) {
       if (!value instanceof File.DirectoryIterator.Entry) {
         throw new TypeError("parameter of " +
           "File.DirectoryIterator.Entry.toMsg must be a " +
           "File.DirectoryIterator.Entry");
       }
       let serialized = {};
       for (let key in File.DirectoryIterator.Entry.prototype) {
         serialized[key] = value[key];
       }
       return serialized;
     };

     let gStatData = new Type.stat.implementation();
     let gStatDataPtr = gStatData.address();
     let MODE_MASK = 4095 ;
     File.Info = function Info(stat) {
       let isDir = (stat.st_mode & Const.S_IFMT) == Const.S_IFDIR;
       let isSymLink = (stat.st_mode & Const.S_IFMT) == Const.S_IFLNK;
       let size = Type.size_t.importFromC(stat.st_size);

       let lastAccessDate = new Date(stat.st_atime * 1000);
       let lastModificationDate = new Date(stat.st_mtime * 1000);
       let unixLastStatusChangeDate = new Date(stat.st_ctime * 1000);

       let unixOwner = Type.uid_t.importFromC(stat.st_uid);
       let unixGroup = Type.gid_t.importFromC(stat.st_gid);
       let unixMode = Type.mode_t.importFromC(stat.st_mode & MODE_MASK);

       SysAll.AbstractInfo.call(this, isDir, isSymLink, size, lastAccessDate,
           lastModificationDate, unixLastStatusChangeDate,
           unixOwner, unixGroup, unixMode);

       
       if ("OSFILE_OFFSETOF_STAT_ST_BIRTHTIME" in Const) {
         let date = new Date(stat.st_birthtime * 1000);

        








         this.macBirthDate = date;
       }
     };
     File.Info.prototype = Object.create(SysAll.AbstractInfo.prototype);

     
     Object.defineProperty(File.Info.prototype, "creationDate", {
      get: function creationDate() {
        
        
        
        return this.macBirthDate || new Date(0);
      }
     });

     




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

     











     File.stat = function stat(path, options = {}) {
       if (options.unixNoFollowingLinks) {
         throw_on_negative("stat", UnixFile.lstat(path, gStatDataPtr));
       } else {
         throw_on_negative("stat", UnixFile.stat(path, gStatDataPtr));
       }
       return new File.Info(gStatData);
     };

     File.read = exports.OS.Shared.AbstractFile.read;
     File.writeAtomic = exports.OS.Shared.AbstractFile.writeAtomic;
     File.openUnique = exports.OS.Shared.AbstractFile.openUnique;
     File.removeDir = exports.OS.Shared.AbstractFile.removeDir;

     


     File.getCurrentDirectory = function getCurrentDirectory() {
       let path = UnixFile.get_current_dir_name?UnixFile.get_current_dir_name():
         UnixFile.getwd_auto(null);
       throw_on_null("getCurrentDirectory",path);
       return path.readString();
     };

     


     File.setCurrentDirectory = function setCurrentDirectory(path) {
       throw_on_negative("setCurrentDirectory",
         UnixFile.chdir(path)
       );
     };

     


     Object.defineProperty(File, "curDir", {
         set: function(path) {
           this.setCurrentDirectory(path);
         },
         get: function() {
           return this.getCurrentDirectory();
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
     File.Error = SysAll.Error;
     exports.OS.File = File;
     exports.OS.Shared.Type = Type;

     Object.defineProperty(File, "POS_START", { value: SysAll.POS_START });
     Object.defineProperty(File, "POS_CURRENT", { value: SysAll.POS_CURRENT });
     Object.defineProperty(File, "POS_END", { value: SysAll.POS_END });
   })(this);
}
