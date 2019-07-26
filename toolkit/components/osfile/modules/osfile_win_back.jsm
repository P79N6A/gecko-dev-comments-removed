




















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
     let advapi32 = new SharedAll.Library("advapi32", "advapi32.dll");
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

       


       Type.PSID =
         Type.voidptr_t.withName("PSID");

       Type.PACL =
         Type.voidptr_t.withName("PACL");

       Type.PSECURITY_DESCRIPTOR =
         Type.voidptr_t.withName("PSECURITY_DESCRIPTOR");

       


       Type.HLOCAL =
         Type.voidptr_t.withName("HLOCAL");

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

       
       
       libc.declareLazy(SysFile, "_CloseHandle",
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

       libc.declareLazy(SysFile, "_FindClose",
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

       

       libc.declareLazyFFI(SysFile, "CopyFile",
         "CopyFileW", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                     Type.path,
                       Type.path,
                    Type.bool);

       libc.declareLazyFFI(SysFile, "CreateDirectory",
         "CreateDirectoryW", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.jschar.in_ptr,
                    Type.SECURITY_ATTRIBUTES.in_ptr);

       libc.declareLazyFFI(SysFile, "CreateFile",
         "CreateFileW", ctypes.winapi_abi,
                      Type.file_HANDLE,
                        Type.path,
                      Type.DWORD,
                       Type.DWORD,
                    Type.SECURITY_ATTRIBUTES.in_ptr,
                    Type.DWORD,
                       Type.DWORD,
                    Type.HANDLE);

       libc.declareLazyFFI(SysFile, "DeleteFile",
         "DeleteFileW", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.path);

       libc.declareLazyFFI(SysFile, "FileTimeToSystemTime",
         "FileTimeToSystemTime", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                    Type.FILETIME.in_ptr,
                     Type.SystemTime.out_ptr);

       libc.declareLazyFFI(SysFile, "SystemTimeToFileTime",
         "SystemTimeToFileTime", ctypes.winapi_abi,
                       Type.zero_or_nothing,
                      Type.SystemTime.in_ptr,
                     Type.FILETIME.out_ptr);

       libc.declareLazyFFI(SysFile, "FindFirstFile",
         "FindFirstFileW", ctypes.winapi_abi,
                     Type.find_HANDLE,
                    Type.path,
                       Type.FindData.out_ptr);

       libc.declareLazyFFI(SysFile, "FindNextFile",
         "FindNextFileW", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.find_HANDLE,
                       Type.FindData.out_ptr);

       libc.declareLazyFFI(SysFile, "FormatMessage",
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

       libc.declareLazyFFI(SysFile, "GetCurrentDirectory",
         "GetCurrentDirectoryW", ctypes.winapi_abi,
                     Type.zero_or_DWORD,
                     Type.DWORD,
                        Type.out_path
                   );

       libc.declareLazyFFI(SysFile, "GetFileInformationByHandle",
         "GetFileInformationByHandle", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                     Type.HANDLE,
                       Type.FILE_INFORMATION.out_ptr);

       libc.declareLazyFFI(SysFile, "MoveFileEx",
         "MoveFileExW", ctypes.winapi_abi,
                       Type.zero_or_nothing,
                     Type.path,
                     Type.path,
                        Type.DWORD
                   );

       libc.declareLazyFFI(SysFile, "ReadFile",
         "ReadFile", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.HANDLE,
                     Type.voidptr_t,
                     Type.DWORD,
                    Type.DWORD.out_ptr,
                    Type.void_t.inout_ptr 
         );

       libc.declareLazyFFI(SysFile, "RemoveDirectory",
         "RemoveDirectoryW", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.path);

       libc.declareLazyFFI(SysFile, "SetCurrentDirectory",
         "SetCurrentDirectoryW", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.path
                   );

       libc.declareLazyFFI(SysFile, "SetEndOfFile",
         "SetEndOfFile", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.HANDLE);

       libc.declareLazyFFI(SysFile, "SetFilePointer",
         "SetFilePointer", ctypes.winapi_abi,
                     Type.negative_or_DWORD,
                       Type.HANDLE,
                    Type.long,
                     Type.long.in_ptr,
                     Type.DWORD);

       libc.declareLazyFFI(SysFile, "SetFileTime",
         "SetFileTime",  ctypes.winapi_abi,
                       Type.zero_or_nothing,
                         Type.HANDLE,
                     Type.FILETIME.in_ptr,
                       Type.FILETIME.in_ptr,
                        Type.FILETIME.in_ptr);


       libc.declareLazyFFI(SysFile, "WriteFile",
         "WriteFile", ctypes.winapi_abi,
                     Type.zero_or_nothing,
                       Type.HANDLE,
                     Type.voidptr_t,
                     Type.DWORD,
                    Type.DWORD.out_ptr,
                    Type.void_t.inout_ptr 
         );

        libc.declareLazyFFI(SysFile, "FlushFileBuffers",
          "FlushFileBuffers", ctypes.winapi_abi,
                      Type.zero_or_nothing,
                        Type.HANDLE);

        libc.declareLazyFFI(SysFile, "GetFileAttributes",
          "GetFileAttributesW", ctypes.winapi_abi,
                        Type.DWORD,
                      Type.path);

        libc.declareLazyFFI(SysFile, "SetFileAttributes",
          "SetFileAttributesW", ctypes.winapi_abi,
                              Type.zero_or_nothing,
                            Type.path,
                      Type.DWORD);

        advapi32.declareLazyFFI(SysFile, "GetNamedSecurityInfo",
          "GetNamedSecurityInfoW", ctypes.winapi_abi,
                            Type.DWORD,
                        Type.path,
                        Type.DWORD,
                      Type.DWORD,
                          Type.PSID.out_ptr,
                          Type.PSID.out_ptr,
                              Type.PACL.out_ptr,
                              Type.PACL.out_ptr,
                      Type.PSECURITY_DESCRIPTOR.out_ptr);

        advapi32.declareLazyFFI(SysFile, "SetNamedSecurityInfo",
          "SetNamedSecurityInfoW", ctypes.winapi_abi,
                            Type.DWORD,
                        Type.path,
                        Type.DWORD,
                      Type.DWORD,
                          Type.PSID,
                          Type.PSID,
                              Type.PACL,
                              Type.PACL);

        libc.declareLazyFFI(SysFile, "LocalFree",
          "LocalFree", ctypes.winapi_abi,
                            Type.HLOCAL,
                               Type.HLOCAL);
     };

     exports.OS.Win = {
       File: {
           _init:  init
       }
     };
   })(this);
}
