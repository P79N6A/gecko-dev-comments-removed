




















{
  if (typeof Components != "undefined") {
    
    
    
    

    throw new Error("osfile_win.jsm cannot be used from the main thread yet");
  }

  (function(exports) {
     "use strict";
     if (exports.OS && exports.OS.Win && exports.OS.Win.File) {
       return; 
     }

     let SharedAll = require("resource://gre/modules/osfile/osfile_shared_allthreads.jsm");
     let SysAll = require("resource://gre/modules/osfile/osfile_win_allthreads.jsm");
     let LOG = SharedAll.LOG.bind(SharedAll, "Unix", "back");
     let libc = SysAll.libc;
     let Const = SharedAll.Constants.Win;

     




     
     let init = function init(aDeclareFFI) {
       let declareFFI;
       if (aDeclareFFI) {
         declareFFI = aDeclareFFI.bind(null, libc);
       } else {
         declareFFI = SysAll.declareFFI;
       }
       let declareLazyFFI = SharedAll.declareLazyFFI;

       
       
       
       let Type = Object.create(SysAll.Type);
       let SysFile = exports.OS.Win.File = { Type: Type };

       

       



       Type.HANDLE =
         Type.voidptr_t.withName("HANDLE");
       Type.HANDLE.importFromC = function importFromC(maybe) {
         if (Type.int.cast(maybe).value == INVALID_HANDLE) {
           
           
           
           return INVALID_HANDLE;
         }
         return ctypes.CDataFinalizer(maybe, this.finalizeHANDLE);
       };
       Type.HANDLE.finalizeHANDLE = function placeholder() {
         throw new Error("finalizeHANDLE should be implemented");
       };
       let INVALID_HANDLE = Const.INVALID_HANDLE_VALUE;

       Type.file_HANDLE = Type.HANDLE.withName("file HANDLE");
       SharedAll.defineLazyGetter(Type.file_HANDLE,
         "finalizeHANDLE",
         function() {
           return SysFile._CloseHandle;
         });

       Type.find_HANDLE = Type.HANDLE.withName("find HANDLE");
       SharedAll.defineLazyGetter(Type.find_HANDLE,
         "finalizeHANDLE",
         function() {
           return SysFile._FindClose;
         });

       Type.DWORD = Type.int32_t.withName("DWORD");

       



       Type.negative_or_DWORD =
         Type.DWORD.withName("negative_or_DWORD");

       



       Type.zero_or_DWORD =
         Type.DWORD.withName("zero_or_DWORD");

       



       Type.zero_or_nothing =
         Type.int.withName("zero_or_nothing");

       Type.SECURITY_ATTRIBUTES =
         Type.void_t.withName("SECURITY_ATTRIBUTES");

       Type.FILETIME =
         new SharedAll.Type("FILETIME",
                  ctypes.StructType("FILETIME", [
                  { lo: Type.DWORD.implementation },
                  { hi: Type.DWORD.implementation }]));

       Type.FindData =
         new SharedAll.Type("FIND_DATA",
                  ctypes.StructType("FIND_DATA", [
                    { dwFileAttributes: ctypes.uint32_t },
                    { ftCreationTime:   Type.FILETIME.implementation },
                    { ftLastAccessTime: Type.FILETIME.implementation },
                    { ftLastWriteTime:  Type.FILETIME.implementation },
                    { nFileSizeHigh:    Type.DWORD.implementation },
                    { nFileSizeLow:     Type.DWORD.implementation },
                    { dwReserved0:      Type.DWORD.implementation },
                    { dwReserved1:      Type.DWORD.implementation },
                    { cFileName:        ctypes.ArrayType(ctypes.jschar, Const.MAX_PATH) },
                    { cAlternateFileName: ctypes.ArrayType(ctypes.jschar, 14) }
                      ]));

       Type.FILE_INFORMATION =
         new SharedAll.Type("FILE_INFORMATION",
                  ctypes.StructType("FILE_INFORMATION", [
                    { dwFileAttributes: ctypes.uint32_t },
                    { ftCreationTime:   Type.FILETIME.implementation },
                    { ftLastAccessTime: Type.FILETIME.implementation },
                    { ftLastWriteTime:  Type.FILETIME.implementation },
                    { dwVolumeSerialNumber: ctypes.uint32_t },
                    { nFileSizeHigh:    Type.DWORD.implementation },
                    { nFileSizeLow:     Type.DWORD.implementation },
                    { nNumberOfLinks:   ctypes.uint32_t },
                    { nFileIndex: ctypes.uint64_t }
                   ]));

       Type.SystemTime =
         new SharedAll.Type("SystemTime",
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

       
       
       SharedAll.declareLazy(SysFile, "_CloseHandle", libc,
                      "CloseHandle", ctypes.winapi_abi,
                        ctypes.bool,
                         ctypes.voidptr_t);

       SysFile.CloseHandle = function(fd) {
         if (fd == INVALID_HANDLE) {
           return true;
         } else {
           return fd.dispose(); 
         }
       };

       SharedAll.declareLazy(SysFile, "_FindClose", libc,
                      "FindClose", ctypes.winapi_abi,
                        ctypes.bool,
                         ctypes.voidptr_t);

       SysFile.FindClose = function(handle) {
         if (handle == INVALID_HANDLE) {
           return true;
         } else {
           return handle.dispose(); 
         }
       };

       

       declareLazyFFI(SysFile, "CopyFile", libc,
         "CopyFileW", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                     Type.path,
                       Type.path,
                    Type.bool);

       declareLazyFFI(SysFile, "CreateDirectory", libc,
         "CreateDirectoryW", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.jschar.in_ptr,
                    Type.SECURITY_ATTRIBUTES.in_ptr);

       declareLazyFFI(SysFile, "CreateFile", libc,
         "CreateFileW", ctypes.winapi_abi,
                      Type.file_HANDLE,
                        Type.path,
                      Type.DWORD,
                       Type.DWORD,
                    Type.SECURITY_ATTRIBUTES.in_ptr,
                    Type.DWORD,
                       Type.DWORD,
                    Type.HANDLE);

       declareLazyFFI(SysFile, "DeleteFile", libc,
         "DeleteFileW", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.path);

       declareLazyFFI(SysFile, "FileTimeToSystemTime", libc,
         "FileTimeToSystemTime", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                    Type.FILETIME.in_ptr,
                     Type.SystemTime.out_ptr);

       declareLazyFFI(SysFile, "FindFirstFile", libc,
         "FindFirstFileW", ctypes.winapi_abi,
                     Type.find_HANDLE,
                    Type.path,
                       Type.FindData.out_ptr);

       declareLazyFFI(SysFile, "FindNextFile", libc,
         "FindNextFileW", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.find_HANDLE,
                       Type.FindData.out_ptr);

       declareLazyFFI(SysFile, "FormatMessage", libc,
         "FormatMessageW", ctypes.winapi_abi,
                     Type.DWORD,
                      Type.DWORD,
                     Type.void_t.in_ptr,
                      Type.DWORD,
                     Type.DWORD,
                        Type.out_wstring,
                       Type.DWORD,
                    Type.void_t.in_ptr
                   );

       declareLazyFFI(SysFile, "GetCurrentDirectory", libc,
         "GetCurrentDirectoryW", ctypes.winapi_abi,
                     Type.zero_or_DWORD,
                     Type.DWORD,
                        Type.out_path
                   );

       declareLazyFFI(SysFile, "GetFileInformationByHandle", libc,
         "GetFileInformationByHandle", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                     Type.HANDLE,
                       Type.FILE_INFORMATION.out_ptr);

       declareLazyFFI(SysFile, "MoveFileEx", libc,
         "MoveFileExW", ctypes.winapi_abi,
                       Type.zero_or_nothing,
                     Type.path,
                     Type.path,
                        Type.DWORD
                   );

       declareLazyFFI(SysFile, "ReadFile", libc,
         "ReadFile", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.HANDLE,
                     Type.voidptr_t,
                     Type.DWORD,
                    Type.DWORD.out_ptr,
                    Type.void_t.inout_ptr 
         );

       declareLazyFFI(SysFile, "RemoveDirectory", libc,
         "RemoveDirectoryW", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.path);

       declareLazyFFI(SysFile, "SetCurrentDirectory", libc,
         "SetCurrentDirectoryW", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.path
                   );

       declareLazyFFI(SysFile, "SetEndOfFile", libc,
         "SetEndOfFile", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.HANDLE);

       declareLazyFFI(SysFile, "SetFilePointer", libc,
         "SetFilePointer", ctypes.winapi_abi,
                     Type.negative_or_DWORD,
                       Type.HANDLE,
                    Type.long,
                     Type.long.in_ptr,
                     Type.DWORD);

       declareLazyFFI(SysFile, "WriteFile", libc,
         "WriteFile", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.HANDLE,
                     Type.voidptr_t,
                     Type.DWORD,
                    Type.DWORD.out_ptr,
                    Type.void_t.inout_ptr 
         );

        declareLazyFFI(SysFile, "FlushFileBuffers", libc,
          "FlushFileBuffers", ctypes.winapi_abi,
                      Type.zero_or_nothing,
                        Type.HANDLE);
     };

     exports.OS.Win = {
       File: {
           _init:  init
       }
     };
   })(this);
}
