










{
  if (typeof Components != "undefined") {
    
    
    throw new Error("osfile_win_front.jsm cannot be used from the main thread yet");
  }

  importScripts("resource://gre/modules/osfile/osfile_win_back.jsm");
  importScripts("resource://gre/modules/osfile/ospath_win_back.jsm");
  importScripts("resource://gre/modules/osfile/osfile_shared_front.jsm");

  (function(exports) {
     "use strict";

     
     if (exports.OS.File) {
       return; 
     }
     exports.OS.Win.File._init();
     let Const = exports.OS.Constants.Win;
     let WinFile = exports.OS.Win.File;
     let LOG = OS.Shared.LOG.bind(OS.Shared, "Win front-end");

     
     
     
     
     
     
     
     
     
     let gBytesRead = new ctypes.int32_t(-1);
     let gBytesReadPtr = gBytesRead.address();
     let gBytesWritten = new ctypes.int32_t(-1);
     let gBytesWrittenPtr = gBytesWritten.address();

     
     let gFileInfo = new OS.Shared.Type.FILE_INFORMATION.implementation();
     let gFileInfoPtr = gFileInfo.address();

     








     let File = function File(fd) {
       exports.OS.Shared.AbstractFile.call(this, fd);
       this._closeResult = null;
     };
     File.prototype = Object.create(exports.OS.Shared.AbstractFile.prototype);

     









     File.prototype.close = function close() {
       if (this._fd) {
         let fd = this._fd;
         this._fd = null;
         
         
         
         let result = WinFile._CloseHandle(fd);
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

     














     File.prototype._read = function _read(buffer, nbytes, options) {
       
       throw_on_zero("read",
         WinFile.ReadFile(this.fd, buffer, nbytes, gBytesReadPtr, null)
       );
       return gBytesRead.value;
     };

     












     File.prototype._write = function _write(buffer, nbytes, options) {
       
       throw_on_zero("write",
         WinFile.WriteFile(this.fd, buffer, nbytes, gBytesWrittenPtr, null)
       );
       return gBytesWritten.value;
     };

     


     File.prototype.getPosition = function getPosition(pos) {
       return this.setPosition(0, File.POS_CURRENT);
     };

     















     File.prototype.setPosition = function setPosition(pos, whence) {
       if (whence === undefined) {
         whence = Const.FILE_BEGIN;
       }
       return throw_on_negative("setPosition",
         WinFile.SetFilePointer(this.fd, pos, null, whence));
     };

     




     File.prototype.stat = function stat() {
       throw_on_zero("stat",
         WinFile.GetFileInformationByHandle(this.fd, gFileInfoPtr));
       return new File.Info(gFileInfo);
     };

     





     File.prototype.flush = function flush() {
       throw_on_zero("flush", WinFile.FlushFileBuffers(this.fd));
     };

     
     const noOptions = {};

     
     
     
     
     const DEFAULT_SHARE = Const.FILE_SHARE_READ |
       Const.FILE_SHARE_WRITE | Const.FILE_SHARE_DELETE;

     
     const DEFAULT_FLAGS = Const.FILE_ATTRIBUTE_NORMAL;

     





















































     File.open = function Win_open(path, mode, options) {
       options = options || noOptions;
       mode = mode || noOptions;
       let share = options.winShare || DEFAULT_SHARE;
       let security = options.winSecurity || null;
       let flags = options.winFlags || DEFAULT_FLAGS;
       let template = options.winTemplate?options.winTemplate._fd:null;
       let access;
       let disposition;
       if ("winAccess" in options && "winDisposition" in options) {
         access = options.winAccess;
         disposition = options.winDisposition;
       } else if (("winAccess" in options && !("winDisposition" in options))
                 ||(!("winAccess" in options) && "winDisposition" in options)) {
         throw new TypeError("OS.File.open requires either both options " +
           "winAccess and winDisposition or neither");
       } else {
         mode = OS.Shared.AbstractFile.normalizeOpenMode(mode);
         if (mode.read) {
           access |= Const.GENERIC_READ;
         }
         if (mode.write) {
           access |= Const.GENERIC_WRITE;
         }
         
         if (mode.trunc) {
           if (mode.existing) {
             
             
             
             disposition = Const.OPEN_EXISTING;
           } else {
             disposition = Const.CREATE_ALWAYS;
           }
         } else if (mode.create) {
           disposition = Const.CREATE_NEW;
         } else if (mode.read && !mode.write) {
           disposition = Const.OPEN_EXISTING;
         } else  {
           if (mode.existing) {
             disposition = Const.OPEN_EXISTING;
           } else {
             disposition = Const.OPEN_ALWAYS;
           }
         }
       }
       let file = error_or_file(WinFile.CreateFile(path,
         access, share, security, disposition, flags, template));
       if (!(mode.trunc && mode.existing)) {
         return file;
       }
       
       file.setPosition(0, File.POS_START);
       throw_on_zero("open",
         WinFile.SetEndOfFile(file.fd));
       return file;
     };

     





     File.remove = function remove(path) {
       throw_on_zero("remove",
         WinFile.DeleteFile(path));
     };

     







     File.removeEmptyDir = function removeEmptyDir(path, options) {
       options = options || noOptions;
       let result = WinFile.RemoveDirectory(path);
       if (!result) {
         if (options.ignoreAbsent &&
             ctypes.winLastError == Const.ERROR_FILE_NOT_FOUND) {
           return;
         }
         throw new File.Error("removeEmptyDir");
       }
     };

     











     File.makeDir = function makeDir(path, options) {
       options = options || noOptions;
       let security = options.winSecurity || null;
       throw_on_zero("makeDir",
         WinFile.CreateDirectory(path, security));
     };

     






















     File.copy = function copy(sourcePath, destPath, options) {
       options = options || noOptions;
       throw_on_zero("copy",
         WinFile.CopyFile(sourcePath, destPath, options.noOverwrite || false)
       );
     };

     






















     File.move = function move(sourcePath, destPath, options) {
       options = options || noOptions;
       let flags;
       if (options.noOverwrite) {
         flags = Const.MOVEFILE_COPY_ALLOWED;
       } else {
         flags = Const.MOVEFILE_COPY_ALLOWED | Const.MOVEFILE_REPLACE_EXISTING;
       }
       throw_on_zero("move",
         WinFile.MoveFileEx(sourcePath, destPath, flags)
       );
     };

     



     let gFindData = new OS.Shared.Type.FindData.implementation();
     let gFindDataPtr = gFindData.address();

     


     let gSystemTime = new OS.Shared.Type.SystemTime.implementation();
     let gSystemTimePtr = gSystemTime.address();

     


     let FILETIME_to_Date = function FILETIME_to_Date(fileTime) {
       if (fileTime == null) {
         throw new TypeError("Expecting a non-null filetime");
       }
       throw_on_zero("FILETIME_to_Date",
                     WinFile.FileTimeToSystemTime(fileTime.address(),
                                                  gSystemTimePtr));
       
       
       let utc = Date.UTC(gSystemTime.wYear,
                          gSystemTime.wMonth - 1
                          ,
                          gSystemTime.wDay, gSystemTime.wHour,
                          gSystemTime.wMinute, gSystemTime.wSecond,
                          gSystemTime.wMilliSeconds);
       return new Date(utc);
     };

     











     File.DirectoryIterator = function DirectoryIterator(path, options) {
       if (options && options.winPattern) {
         this._pattern = path + "\\" + options.winPattern;
       } else {
         this._pattern = path + "\\*";
       }
       this._handle = null;
       this._path = path;
       this._started = false;
     };
     File.DirectoryIterator.prototype = {
       __iterator__: function __iterator__() {
         return this;
       },

       




       _next: function _next() {
         
         
         if (!this._started) {
            this._started = true;
            this._handle = WinFile.FindFirstFile(this._pattern, gFindDataPtr);
            if (this._handle == null) {
              let error = ctypes.winLastError;
              if (error == Const.ERROR_FILE_NOT_FOUND) {
                this.close();
                return null;
              } else {
                throw new File.Error("iter (FindFirstFile)", error);
              }
            }
            return gFindData;
         }

         
         if (!this._handle) {
           return null;
         }

         if (WinFile.FindNextFile(this._handle, gFindDataPtr)) {
           return gFindData;
         } else {
           let error = ctypes.winLastError;
           this.close();
           if (error == Const.ERROR_NO_MORE_FILES) {
              return null;
           } else {
              throw new File.Error("iter (FindNextFile)", error);
           }
         }
       },
       









       next: function next() {
         
         
         
         for (let entry = this._next(); entry != null; entry = this._next()) {
           let name = entry.cFileName.readString();
           if (name == "." || name == "..") {
             continue;
           }
           return new File.DirectoryIterator.Entry(entry, this._path);
         }
         throw StopIteration;
       },
       close: function close() {
         if (!this._handle) {
           return;
         }
         WinFile.FindClose(this._handle);
         this._handle = null;
       }
     };
     File.DirectoryIterator.Entry = function Entry(win_entry, parent) {
       
       
       if (!win_entry.dwFileAttributes) {
         throw new TypeError();
       }
       this._dwFileAttributes = win_entry.dwFileAttributes;
       this._name = win_entry.cFileName.readString();
       if (!this._name) {
         throw new TypeError("Empty name");
       }
       this._ftCreationTime = win_entry.ftCreationTime;
       if (!win_entry.ftCreationTime) {
         throw new TypeError();
       }
       this._ftLastAccessTime = win_entry.ftLastAccessTime;
       if (!win_entry.ftLastAccessTime) {
         throw new TypeError();
       }
       this._ftLastWriteTime = win_entry.ftLastWriteTime;
       if (!win_entry.ftLastWriteTime) {
         throw new TypeError();
       }
       if (!parent) {
         throw new TypeError("Empty parent");
       }
       this._parent = parent;
     };
     File.DirectoryIterator.Entry.prototype = {
       


       get isDir() {
         return !!(this._dwFileAttributes & Const.FILE_ATTRIBUTE_DIRECTORY);
       },
       


       get isSymLink() {
         return !!(this._dwFileAttributes & Const.FILE_ATTRIBUTE_REPARSE_POINT);
       },
       



       get name() {
         return this._name;
       },
       



       get winCreationDate() {
         let date = FILETIME_to_Date(this._ftCreationTime);
         delete this.winCreationDate;
         Object.defineProperty(this, "winCreationDate", {value: date});
         return date;
       },
       



       get winLastWriteDate() {
         let date = FILETIME_to_Date(this._ftLastWriteTime);
         delete this.winLastWriteDate;
         Object.defineProperty(this, "winLastWriteDate", {value: date});
         return date;
       },
       



       get winLastAccessDate() {
         let date = FILETIME_to_Date(this._ftLastAccessTime);
         delete this.winLastAccessDate;
         Object.defineProperty(this, "winLastAccessDate", {value: date});
         return date;
       },
       



       get path() {
         delete this.path;
         let path = OS.Win.Path.join(this._parent, this.name);
         Object.defineProperty(this, "path", {value: path});
         return path;
       }
     };

     






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


     








     File.Info = function Info(stat) {
       this._dwFileAttributes = stat.dwFileAttributes;
       this._ftCreationTime = stat.ftCreationTime;
       this._ftLastAccessTime = stat.ftLastAccessTime;
       this._ftLastWriteTime = stat.ftLastAccessTime;
       this._nFileSizeHigh = stat.nFileSizeHigh;
       this._nFileSizeLow = stat.nFileSizeLow;
     };
     File.Info.prototype = {
       


       get isDir() {
         return !!(this._dwFileAttributes & Const.FILE_ATTRIBUTE_DIRECTORY);
       },
       


       get isSymLink() {
         return !!(this._dwFileAttributes & Const.FILE_ATTRIBUTE_REPARSE_POINT);
       },
       







       get size() {
         let value = ctypes.UInt64.join(this._nFileSizeHigh, this._nFileSizeLow);
         return exports.OS.Shared.Type.uint64_t.importFromC(value);
       },
       




       get creationDate() {
         delete this.creationDate;
         let date = FILETIME_to_Date(this._ftCreationTime);
         Object.defineProperty(this, "creationDate", { value: date });
         return date;
       },
       







       get lastAccessDate() {
         delete this.lastAccess;
         let date = FILETIME_to_Date(this._ftLastAccessTime);
         Object.defineProperty(this, "lastAccessDate", { value: date });
         return date;
       },
       







       get lastModificationDate() {
         delete this.lastModification;
         let date = FILETIME_to_Date(this._ftLastWriteTime);
         Object.defineProperty(this, "lastModificationDate", { value: date });
         return date;
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


     












     File.stat = function stat(path) {
       let file = File.open(path, FILE_STAT_MODE, FILE_STAT_OPTIONS);
       try {
         return file.stat();
       } finally {
         file.close();
       }
     };
     
     
     const FILE_STAT_MODE = {
       read:true
     };
     const FILE_STAT_OPTIONS = {
       
       winAccess: 0,
       
       winFlags: OS.Constants.Win.FILE_FLAG_BACKUP_SEMANTICS,
       winDisposition: OS.Constants.Win.OPEN_EXISTING
     };

     


     Object.defineProperty(File, "curDir", {
         set: function(path) {
           throw_on_zero("set curDir",
             WinFile.SetCurrentDirectory(path));
         },
         get: function() {
           
           
           
           
           
           
           
           
           
           
           
           

           let buffer_size = 4096;
           while (true) {
             let array = new (ctypes.ArrayType(ctypes.jschar, buffer_size))();
             let expected_size = throw_on_zero("get curDir",
               WinFile.GetCurrentDirectory(buffer_size, array)
             );
             if (expected_size <= buffer_size) {
               return array.readString();
             }
             
             
             

             
             
             buffer_size = expected_size;
           }
         }
       }
     );

     
     function error_or_file(maybe) {
       if (maybe == exports.OS.Constants.Win.INVALID_HANDLE_VALUE) {
         throw new File.Error("open");
       }
       return new File(maybe);
     }
     function throw_on_zero(operation, result) {
       if (result == 0) {
         throw new File.Error(operation);
       }
       return result;
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

     File.Win = exports.OS.Win.File;
     File.Error = exports.OS.Shared.Win.Error;
     exports.OS.File = File;

     exports.OS.Path = exports.OS.Win.Path;

     Object.defineProperty(File, "POS_START", { value: OS.Shared.POS_START });
     Object.defineProperty(File, "POS_CURRENT", { value: OS.Shared.POS_CURRENT });
     Object.defineProperty(File, "POS_END", { value: OS.Shared.POS_END });
   })(this);
}
