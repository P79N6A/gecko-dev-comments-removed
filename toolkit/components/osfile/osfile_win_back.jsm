




















{
  if (typeof Components != "undefined") {
    
    
    
    

    throw new Error("osfile_win.jsm cannot be used from the main thread yet");
  }
  importScripts("resource://gre/modules/osfile/osfile_shared.jsm");

  (function(exports) {
     "use strict";
     if (!exports.OS) {
       exports.OS = {};
     }
     if (!exports.OS.Win) {
       exports.OS.Win = {};
     }
     if (exports.OS.Win.File) {
       return; 
     }
     exports.OS.Win.File = {};

     let LOG = OS.Shared.LOG.bind(OS.Shared, "OS.Win.File");

     let libc = ctypes.open("kernel32.dll");
     if (!libc) {
       throw new Error("Could not open kernel32.dll");
     }

     




     let init = function init(aDeclareFFI) {
       let declareFFI;
       if (aDeclareFFI) {
         declareFFI = aDeclareFFI.bind(null, libc);
       } else {
         declareFFI = OS.Shared.declareFFI.bind(null, libc);
       }

       
       let OSWin = exports.OS.Win;
       let WinFile = exports.OS.Win.File;
       if (!exports.OS.Types) {
         exports.OS.Types = {};
       }
       let Type = exports.OS.Shared.Type;
       let Types = Type;

       

       Types.HANDLE =
         new Type("HANDLE",
                  ctypes.voidptr_t);

       



       Types.maybe_HANDLE =
         new Type("maybe_HANDLE",
           Types.HANDLE.implementation,
           function (maybe) {
             if (ctypes.cast(maybe, ctypes.int).value == invalid_handle) {
               
               
               
               return invalid_handle;
             }
             return ctypes.CDataFinalizer(maybe, _CloseHandle);
           });

       



       Types.maybe_find_HANDLE =
         new Type("maybe_find_HANDLE",
           Types.HANDLE.implementation,
           function (maybe) {
             if (ctypes.cast(maybe, ctypes.int).value == invalid_handle) {
               
               
               
               return invalid_handle;
             }
             return ctypes.CDataFinalizer(maybe, _FindClose);
           });

       let invalid_handle = exports.OS.Constants.Win.INVALID_HANDLE_VALUE;

       Types.DWORD = Types.int32_t;

       



       Types.negative_or_DWORD =
         new Type("negative_or_DWORD",
                  ctypes.int32_t);

       



       Types.zero_or_DWORD =
         new Type("zero_or_DWORD",
                  ctypes.int32_t);

       



       Types.zero_or_nothing =
         new Type("zero_or_nothing",
                  Types.bool.implementation);

       Types.FILETIME =
         new Type("FILETIME",
                  ctypes.StructType("FILETIME", [
                  { lo: Types.DWORD.implementation },
                  { hi: Types.DWORD.implementation }]));
              
       Types.FindData =
         new Type("FIND_DATA",
                  ctypes.StructType("FIND_DATA", [
                    { dwFileAttributes: ctypes.uint32_t },
                    { ftCreationTime:   Types.FILETIME.implementation },
                    { ftLastAccessTime: Types.FILETIME.implementation },
                    { ftLastWriteTime:  Types.FILETIME.implementation },
                    { nFileSizeHigh:    Types.DWORD.implementation },
                    { nFileSizeLow:     Types.DWORD.implementation },
                    { dwReserved0:      Types.DWORD.implementation },
                    { dwReserved1:      Types.DWORD.implementation },
                    { cFileName:        ctypes.ArrayType(ctypes.jschar, exports.OS.Constants.Win.MAX_PATH) },
                    { cAlternateFileName: ctypes.ArrayType(ctypes.jschar, 14) }
                      ]));
                  
       Types.SystemTime =
         new Type("SystemTime",
                  ctypes.StructType("SystemTime", [
                  { wYear:      ctypes.int16_t },
                  { wMonth:     ctypes.int16_t },
                  { wDayOfWeek: ctypes.int16_t },
                  { wDay:       ctypes.int16_t },
                  { wHour:      ctypes.int16_t },
                  { wMinute:    ctypes.int16_t },
                  { wSecond:    ctypes.int16_t },
                  { wMilliSeconds: ctypes.int16_t }
                  ]));

       
       
       let _CloseHandle =
         libc.declare("CloseHandle", ctypes.winapi_abi,
                        ctypes.bool,
                         ctypes.voidptr_t);

       WinFile.CloseHandle = function(fd) {
         return fd.dispose(); 
       };

       let _FindClose =
         libc.declare("CloseHandle", ctypes.winapi_abi,
                        ctypes.bool,
                         ctypes.voidptr_t);

       WinFile.FindClose = function(handle) {
         return handle.dispose(); 
       };

       

       WinFile.CopyFile =
         declareFFI("CopyFileW", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                     Types.jschar.in_ptr,
                       Types.jschar.in_ptr,
                    Types.bool);

       WinFile.CreateFile =
         declareFFI("CreateFileW", ctypes.winapi_abi,
                      Types.maybe_HANDLE,
                        Types.jschar.in_ptr,
                      Types.DWORD,
                       Types.DWORD,
                    Types.void_t.in_ptr,
                    Types.DWORD,
                       Types.DWORD,
                    Types.HANDLE);

       WinFile.DeleteFile =
         declareFFI("DeleteFileW", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                       Types.jschar.in_ptr);

       WinFile.FileTimeToSystemTime =
         declareFFI("FileTimeToSystemTime", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                    Types.FILETIME.in_ptr,
                     Types.SystemTime.out_ptr);

       WinFile.FindFirstFile =
         declareFFI("FindFirstFileW", ctypes.winapi_abi,
                     Types.maybe_find_HANDLE,
                    Types.jschar.in_ptr,
                       Types.FindData.out_ptr);

       WinFile.FindNextFile =
         declareFFI("FindNextFileW", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                       Types.HANDLE,
                       Types.FindData.out_ptr);

       WinFile.FormatMessage =
         declareFFI("FormatMessageW", ctypes.winapi_abi,
                     Types.DWORD,
                      Types.DWORD,
                     Types.void_t.in_ptr,
                      Types.DWORD,
                     Types.DWORD,
                        Types.jschar.out_ptr,
                       Types.DWORD,
                    Types.void_t.in_ptr
                   );

       WinFile.GetCurrentDirectory =
         declareFFI("GetCurrentDirectoryW", ctypes.winapi_abi,
                     Types.zero_or_DWORD,
                     Types.DWORD,
                        Types.jschar.out_ptr
                   );

       WinFile.MoveFileEx =
         declareFFI("MoveFileExW", ctypes.winapi_abi,
                       Types.zero_or_nothing,
                     Types.jschar.in_ptr,
                     Types.jschar.in_ptr,
                        Types.DWORD
                   );

       WinFile.ReadFile =
         declareFFI("ReadFile", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                       Types.HANDLE,
                     Types.char.out_ptr,
                     Types.DWORD,
                    Types.DWORD.out_ptr,
                    Types.void_t.inout_ptr 
         );

       WinFile.RemoveDirectory =
         declareFFI("RemoveDirectoryW", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                       Types.jschar.in_ptr);

       WinFile.SetCurrentDirectory =
         declareFFI("SetCurrentDirectoryW", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                       Types.jschar.in_ptr
                   );

       WinFile.SetEndOfFile =
         declareFFI("SetEndOfFile", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                       Types.HANDLE);

       WinFile.SetFilePointer =
         declareFFI("SetFilePointer", ctypes.winapi_abi,
                     Types.negative_or_DWORD,
                       Types.HANDLE,
                    Types.long,
                     Types.long.in_ptr,
                     Types.DWORD);

       WinFile.WriteFile =
         declareFFI("WriteFile", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                       Types.HANDLE,
                     Types.char.in_ptr,
                     Types.DWORD,
                    Types.DWORD.out_ptr,
                    Types.void_t.inout_ptr 
         );

       

       exports.OS.Win.libc = libc;
       exports.OS.Win.declareFFI = declareFFI;
     };
     exports.OS.Win.File._init = init;
   })(this);
}
