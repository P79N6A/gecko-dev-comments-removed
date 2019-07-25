










{
  if (typeof Components != "undefined") {
    
    
    throw new Error("osfile_win_front.jsm cannot be used from the main thread yet");
  }

  importScripts("resource://gre/modules/osfile/osfile_shared.jsm");
  importScripts("resource://gre/modules/osfile/osfile_win_back.jsm");

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

     








     let File = function File(fd) {
       this._fd = fd;
     };
     File.prototype = {
       



       get fd() {
         return this._fd;
       },

       
       
       _nofd: function nofd(operation) {
         operation = operation ||
             this._nofd.caller.name ||
             "unknown operation";
         throw new File.Error(operation, Const.INVALID_HANDLE_VALUE);
       },

       









       close: function close() {
         if (this._fd) {
           let fd = this._fd;
           this._fd = null;
           delete this.fd;
           Object.defineProperty(this, "fd", {get: File.prototype._nofd});
           
           if (fd.dispose() == 0) {
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
         
         throw_on_zero("read",
           WinFile.ReadFile(this.fd, buffer, nbytes, gBytesReadPtr, null)
         );
         return gBytesRead.value;
       },

       












       write: function write(buffer, nbytes, options) {
         
         throw_on_zero("write",
           WinFile.WriteFile(this.fd, buffer, nbytes, gBytesWrittenPtr, null)
         );
         return gBytesWritten.value;
       },

       


       getPosition: function getPosition(pos) {
         return this.setPosition(0, File.POS_CURRENT);
       },

       















       setPosition: function setPosition(pos, whence) {
         
         
         
         
         
         whence = (whence == undefined)?Const.FILE_BEGIN:whence;
         return throw_on_negative("setPosition",
	   WinFile.SetFilePointer(this.fd, pos, null, whence));
       }
     };

     






















     File.Error = function FileError(operation, lastError) {
       operation = operation || File.Error.caller.name || "unknown operation";
       OS.Shared.Error.call(this, operation);
       this.winLastError = lastError || ctypes.winLastError;
     };
     File.Error.prototype = new OS.Shared.Error();
     File.Error.prototype.toString = function toString() {
         let buf = new (ctypes.ArrayType(ctypes.jschar, 1024))();
         let result = WinFile.FormatMessage(
           OS.Constants.Win.FORMAT_MESSAGE_FROM_SYSTEM |
           OS.Constants.Win.FORMAT_MESSAGE_IGNORE_INSERTS,
           null,
            this.winLastError,
            0,
                buf,
            1024,
                  null
         );
         if (!result) {
           buf = "additional error " +
             ctypes.winLastError +
             " while fetching system error message";
         }
         return "Win error " + this.winLastError + " during operation "
           + this.operation + " (" + buf.readString() + " )";
     };

     



     Object.defineProperty(File.Error.prototype, "becauseExists", {
       get: function becauseExists() {
         return this.winLastError == OS.Constants.Win.ERROR_FILE_EXISTS;
       }
     });
     



     Object.defineProperty(File.Error.prototype, "becauseNoSuchFile", {
       get: function becauseNoSuchFile() {
         return this.winLastError == OS.Constants.Win.ERROR_FILE_NOT_FOUND;
       }
     });

     
     const noOptions = {};

     
     
     
     
     const DEFAULT_SHARE = Const.FILE_SHARE_READ |
       Const.FILE_SHARE_WRITE | Const.FILE_SHARE_DELETE;

     
     const DEFAULT_FLAGS = Const.FILE_ATTRIBUTE_NORMAL;

     





















































     File.open = function Win_open(path, mode, options) {
       options = options || noOptions;

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
         mode = OS.Shared._aux.normalizeOpenMode(mode);
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




     

     
     File.POS_START = Const.FILE_BEGIN;
     File.POS_CURRENT = Const.FILE_CURRENT;
     File.POS_END = Const.FILE_END;

     File.Win = exports.OS.Win.File;
     exports.OS.File = File;
   })(this);
}
