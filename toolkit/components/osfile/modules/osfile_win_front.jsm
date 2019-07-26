










{
  if (typeof Components != "undefined") {
    
    
    throw new Error("osfile_win_front.jsm cannot be used from the main thread yet");
  }

  (function(exports) {
     "use strict";


      
     if (exports.OS && exports.OS.File) {
        return; 
     }

     let SharedAll = require("resource://gre/modules/osfile/osfile_shared_allthreads.jsm");
     let Path = require("resource://gre/modules/osfile/ospath.jsm");
     let SysAll = require("resource://gre/modules/osfile/osfile_win_allthreads.jsm");
     exports.OS.Win.File._init();
     let Const = exports.OS.Constants.Win;
     let WinFile = exports.OS.Win.File;
     let Type = WinFile.Type;

     
     
     
     
     
     
     
     
     
     let gBytesRead = new ctypes.int32_t(-1);
     let gBytesReadPtr = gBytesRead.address();
     let gBytesWritten = new ctypes.int32_t(-1);
     let gBytesWrittenPtr = gBytesWritten.address();

     
     let gFileInfo = new Type.FILE_INFORMATION.implementation();
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
       if (this._appendMode) {
         
         
         this.setPosition(0, File.POS_END);
       }
       
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

     














     File.prototype.setDates = function setDates(accessDate, modificationDate) {
       accessDate = Date_to_FILETIME("File.prototype.setDates", accessDate);
       modificationDate = Date_to_FILETIME("File.prototype.setDates",
                                           modificationDate);
       throw_on_zero("setDates",
                     WinFile.SetFileTime(this.fd, null, accessDate.address(),
                                         modificationDate.address()));
     };

     










     File.prototype.flush = function flush() {
       throw_on_zero("flush", WinFile.FlushFileBuffers(this.fd));
     };

     
     
     
     
     const DEFAULT_SHARE = Const.FILE_SHARE_READ |
       Const.FILE_SHARE_WRITE | Const.FILE_SHARE_DELETE;

     
     const DEFAULT_FLAGS = Const.FILE_ATTRIBUTE_NORMAL;

     
























































     File.open = function Win_open(path, mode = {}, options = {}) {
       let share = options.winShare !== undefined ? options.winShare : DEFAULT_SHARE;
       let security = options.winSecurity || null;
       let flags = options.winFlags !== undefined ? options.winFlags : DEFAULT_FLAGS;
       let template = options.winTemplate ? options.winTemplate._fd : null;
       let access;
       let disposition;

       mode = OS.Shared.AbstractFile.normalizeOpenMode(mode);

       if ("winAccess" in options && "winDisposition" in options) {
         access = options.winAccess;
         disposition = options.winDisposition;
       } else if (("winAccess" in options && !("winDisposition" in options))
                 ||(!("winAccess" in options) && "winDisposition" in options)) {
         throw new TypeError("OS.File.open requires either both options " +
           "winAccess and winDisposition or neither");
       } else {
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
         } else if (mode.existing) {
           disposition = Const.OPEN_EXISTING;
         } else {
           disposition = Const.OPEN_ALWAYS;
         }
       }

       let file = error_or_file(WinFile.CreateFile(path,
         access, share, security, disposition, flags, template));

       file._appendMode = !!mode.append;

       if (!(mode.trunc && mode.existing)) {
         return file;
       }
       
       file.setPosition(0, File.POS_START);
       throw_on_zero("open",
         WinFile.SetEndOfFile(file.fd));
       return file;
     };

     






     File.exists = function Win_exists(path) {
       try {
         let file = File.open(path, FILE_STAT_MODE, FILE_STAT_OPTIONS);
         file.close();
         return true;
       } catch (x) {
         return false;
       }
     };

     









     File.remove = function remove(path, options = {}) {
       if (WinFile.DeleteFile(path)) {
         return;
       }

       if (ctypes.winLastError == Const.ERROR_FILE_NOT_FOUND) {
         if ((!("ignoreAbsent" in options) || options.ignoreAbsent)) {
           return;
         }
       } else if (ctypes.winLastError == Const.ERROR_ACCESS_DENIED) {
         let attributes = WinFile.GetFileAttributes(path);
         if (attributes != Const.INVALID_FILE_ATTRIBUTES &&
             attributes & Const.FILE_ATTRIBUTE_READONLY) {
           let newAttributes = attributes & ~Const.FILE_ATTRIBUTE_READONLY;
           if (WinFile.SetFileAttributes(path, newAttributes) &&
               WinFile.DeleteFile(path)) {
             return;
           }
         }
       }

       throw new File.Error("remove");
     };

     







     File.removeEmptyDir = function removeEmptyDir(path, options = {}) {
       let result = WinFile.RemoveDirectory(path);
       if (!result) {
         if ((!("ignoreAbsent" in options) || options.ignoreAbsent) &&
             ctypes.winLastError == Const.ERROR_FILE_NOT_FOUND) {
           return;
         }
         throw new File.Error("removeEmptyDir");
       }
     };

     













     File.makeDir = function makeDir(path, options = {}) {
       let security = options.winSecurity || null;
       let result = WinFile.CreateDirectory(path, security);
       if (!result) {
         if ((!("ignoreExisting" in options) || options.ignoreExisting) &&
             ctypes.winLastError == Const.ERROR_ALREADY_EXISTS) {
           return;
         }
         throw new File.Error("makeDir");
       }
     };

     






















     File.copy = function copy(sourcePath, destPath, options = {}) {
       throw_on_zero("copy",
         WinFile.CopyFile(sourcePath, destPath, options.noOverwrite || false)
       );
     };

     

























     File.move = function move(sourcePath, destPath, options = {}) {
       let flags = 0;
       if (!options.noCopy) {
         flags = Const.MOVEFILE_COPY_ALLOWED;
       }
       if (!options.noOverwrite) {
         flags = flags | Const.MOVEFILE_REPLACE_EXISTING;
       }
       throw_on_zero("move",
         WinFile.MoveFileEx(sourcePath, destPath, flags)
       );

       
       
       if (Path.dirname(sourcePath) === Path.dirname(destPath)) {
         
         return;
       }
       
       
       
       
       
       let dacl = new ctypes.voidptr_t();
       let sd = new ctypes.voidptr_t();
       WinFile.GetNamedSecurityInfo(destPath, Const.SE_FILE_OBJECT,
                                    Const.DACL_SECURITY_INFORMATION,
                                    null , null ,
                                    dacl.address(), null ,
                                    sd.address());
       
       if (!dacl.isNull()) {
         WinFile.SetNamedSecurityInfo(destPath, Const.SE_FILE_OBJECT,
                                      Const.DACL_SECURITY_INFORMATION |
                                      Const.UNPROTECTED_DACL_SECURITY_INFORMATION,
                                      null , null ,
                                      dacl, null );
       }
       
       if (!sd.isNull()) {
           WinFile.LocalFree(Type.HLOCAL.cast(sd));
       }
     };

     


     let gSystemTime = new Type.SystemTime.implementation();
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

     







     let Date_to_FILETIME = function Date_to_FILETIME(fn, date) {
       if (typeof date === "number") {
         date = new Date(date);
       } else if (!date) {
         date = new Date();
       } else if (typeof date.getUTCFullYear !== "function") {
         throw new TypeError("|date| parameter of " + fn + " must be a " +
                             "|Date| instance or number");
       }
       gSystemTime.wYear = date.getUTCFullYear();
       
       gSystemTime.wMonth = date.getUTCMonth() + 1;
       gSystemTime.wDay = date.getUTCDate();
       gSystemTime.wHour = date.getUTCHours();
       gSystemTime.wMinute = date.getUTCMinutes();
       gSystemTime.wSecond = date.getUTCSeconds();
       gSystemTime.wMilliseconds = date.getUTCMilliseconds();
       let result = new OS.Shared.Type.FILETIME.implementation();
       throw_on_zero("Date_to_FILETIME",
                     WinFile.SystemTimeToFileTime(gSystemTimePtr,
                                                  result.address()));
       return result;
     };

     













     File.DirectoryIterator = function DirectoryIterator(path, options) {
       exports.OS.Shared.AbstractFile.AbstractIterator.call(this);
       if (options && options.winPattern) {
         this._pattern = path + "\\" + options.winPattern;
       } else {
         this._pattern = path + "\\*";
       }
       this._path = path;

       
       this._first = true;
       this._findData = new Type.FindData.implementation();
       this._findDataPtr = this._findData.address();
       this._handle = WinFile.FindFirstFile(this._pattern, this._findDataPtr);
       if (this._handle == Const.INVALID_HANDLE_VALUE) {
         let error = ctypes.winLastError;
         this._findData = null;
         this._findDataPtr = null;
         if (error == Const.ERROR_FILE_NOT_FOUND) {
           
           SharedAll.LOG("Directory is empty");
           this._closed = true;
           this._exists = true;
         } else if (error == Const.ERROR_PATH_NOT_FOUND) {
           
           SharedAll.LOG("Directory does not exist");
           this._closed = true;
           this._exists = false;
         } else {
           throw new File.Error("DirectoryIterator", error);
         }
       } else {
         this._closed = false;
         this._exists = true;
       }
     };

     File.DirectoryIterator.prototype = Object.create(exports.OS.Shared.AbstractFile.AbstractIterator.prototype);


     




     File.DirectoryIterator.prototype._next = function _next() {
       
       if (!this._exists) {
         throw File.Error.noSuchFile("DirectoryIterator.prototype.next");
       }
       
       if (this._closed) {
         return null;
       }
       
       
       if (this._first) {
         this._first = false;
         return this._findData;
       }

       if (WinFile.FindNextFile(this._handle, this._findDataPtr)) {
         return this._findData;
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

     









     File.DirectoryIterator.prototype.next = function next() {
         
         
         
         for (let entry = this._next(); entry != null; entry = this._next()) {
           let name = entry.cFileName.readString();
           if (name == "." || name == "..") {
             continue;
           }
           return new File.DirectoryIterator.Entry(entry, this._path);
         }
         throw StopIteration;
     };

     File.DirectoryIterator.prototype.close = function close() {
       if (this._closed) {
         return;
       }
       this._closed = true;
       if (this._handle) {
         
         
         throw_on_zero("FindClose",
           WinFile.FindClose(this._handle));
         this._handle = null;
       }
     };

    




     File.DirectoryIterator.prototype.exists = function exists() {
       return this._exists;
     };

     File.DirectoryIterator.Entry = function Entry(win_entry, parent) {
       if (!win_entry.dwFileAttributes || !win_entry.ftCreationTime ||
           !win_entry.ftLastAccessTime || !win_entry.ftLastWriteTime)
        throw new TypeError();

       
       
       let isDir = !!(win_entry.dwFileAttributes & Const.FILE_ATTRIBUTE_DIRECTORY);
       let isSymLink = !!(win_entry.dwFileAttributes & Const.FILE_ATTRIBUTE_REPARSE_POINT);

       let winCreationDate = FILETIME_to_Date(win_entry.ftCreationTime);
       let winLastWriteDate = FILETIME_to_Date(win_entry.ftLastWriteTime);
       let winLastAccessDate = FILETIME_to_Date(win_entry.ftLastAccessTime);

       let name = win_entry.cFileName.readString();
       if (!name) {
         throw new TypeError("Empty name");
       }

       if (!parent) {
         throw new TypeError("Empty parent");
       }
       this._parent = parent;

       let path = Path.join(this._parent, name);

       SysAll.AbstractEntry.call(this, isDir, isSymLink, name,
         winCreationDate, winLastWriteDate,
         winLastAccessDate, path);
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


     








     File.Info = function Info(stat) {
       let isDir = !!(stat.dwFileAttributes & Const.FILE_ATTRIBUTE_DIRECTORY);
       let isSymLink = !!(stat.dwFileAttributes & Const.FILE_ATTRIBUTE_REPARSE_POINT);
       
       let winBirthDate = FILETIME_to_Date(stat.ftCreationTime);
       let lastAccessDate = FILETIME_to_Date(stat.ftLastAccessTime);
       let lastWriteDate = FILETIME_to_Date(stat.ftLastWriteTime);

       let value = ctypes.UInt64.join(stat.nFileSizeHigh, stat.nFileSizeLow);
       let size = Type.uint64_t.importFromC(value);

       SysAll.AbstractInfo.call(this, isDir, isSymLink, size,
         winBirthDate, lastAccessDate,
         lastWriteDate);
     };
     File.Info.prototype = Object.create(SysAll.AbstractInfo.prototype);

     




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
       read: true
     };
     const FILE_STAT_OPTIONS = {
       
       winAccess: 0,
       
       winFlags: Const.FILE_FLAG_BACKUP_SEMANTICS,
       winDisposition: Const.OPEN_EXISTING
     };

     























     File.setDates = function setDates(path, accessDate, modificationDate) {
       let file = File.open(path, FILE_SETDATES_MODE, FILE_SETDATES_OPTIONS);
       try {
         return file.setDates(accessDate, modificationDate);
       } finally {
         file.close();
       }
     };
     
     
     const FILE_SETDATES_MODE = {
       write: true
     };
     const FILE_SETDATES_OPTIONS = {
       winAccess: Const.GENERIC_WRITE,
       
       winFlags: Const.FILE_FLAG_BACKUP_SEMANTICS,
       winDisposition: Const.OPEN_EXISTING
     };

     File.read = exports.OS.Shared.AbstractFile.read;
     File.writeAtomic = exports.OS.Shared.AbstractFile.writeAtomic;
     File.openUnique = exports.OS.Shared.AbstractFile.openUnique;
     File.removeDir = exports.OS.Shared.AbstractFile.removeDir;

     


     File.getCurrentDirectory = function getCurrentDirectory() {
       
       
       
       
       
       
       
       
       
       
       
       
       
       let buffer_size = 4096;
       while (true) {
         let array = new (ctypes.ArrayType(ctypes.jschar, buffer_size))();
         let expected_size = throw_on_zero("getCurrentDirectory",
           WinFile.GetCurrentDirectory(buffer_size, array)
         );
         if (expected_size <= buffer_size) {
           return array.readString();
         }
         
         
         
         
         
         buffer_size = expected_size + 1 ;
       }
     };

     


     File.setCurrentDirectory = function setCurrentDirectory(path) {
       throw_on_zero("setCurrentDirectory",
         WinFile.SetCurrentDirectory(path));
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
       if (maybe == Const.INVALID_HANDLE_VALUE) {
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
     File.Error = SysAll.Error;
     exports.OS.File = File;
     exports.OS.Shared.Type = Type;

     Object.defineProperty(File, "POS_START", { value: SysAll.POS_START });
     Object.defineProperty(File, "POS_CURRENT", { value: SysAll.POS_CURRENT });
     Object.defineProperty(File, "POS_END", { value: SysAll.POS_END });
   })(this);
}
