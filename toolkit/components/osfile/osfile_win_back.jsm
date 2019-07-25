




















{
  if (typeof Components != "undefined") {
    
    
    
    

    throw new Error("osfile_win.jsm cannot be used from the main thread yet");
  }
  importScripts("resource://gre/modules/osfile/osfile_shared_allthreads.jsm");
  importScripts("resource://gre/modules/osfile/osfile_win_allthreads.jsm");

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

     let LOG = OS.Shared.LOG.bind(OS.Shared, "Win", "back");
     let libc = exports.OS.Shared.Win.libc;

     




     
     let init = function init(aDeclareFFI) {
       let declareFFI;
       if (aDeclareFFI) {
         declareFFI = aDeclareFFI.bind(null, libc);
       } else {
         declareFFI = exports.OS.Shared.Win.declareFFI;
       }

       
       let OSWin = exports.OS.Win;
       let WinFile = exports.OS.Win.File;
       if (!exports.OS.Types) {
         exports.OS.Types = {};
       }
       let Type = exports.OS.Shared.Type;
       let Types = Type;

       

       Types.HANDLE =
         Types.voidptr_t.withName("HANDLE");

       



       Types.maybe_HANDLE =
         Types.HANDLE.withName("maybe_HANDLE");
       Types.maybe_HANDLE.importFromC =
         function maybe_HANDLE_importFromC(maybe) {
           if (Types.int.cast(maybe).value == INVALID_HANDLE) {
             
             
             
             return INVALID_HANDLE;
           }
         return ctypes.CDataFinalizer(maybe, _CloseHandle);
         };

       



       Types.maybe_find_HANDLE =
         Types.maybe_HANDLE.withName("maybe_find_HANDLE");

       let INVALID_HANDLE = exports.OS.Constants.Win.INVALID_HANDLE_VALUE;

       Types.DWORD = Types.int32_t.withName("DWORD");

       



       Types.negative_or_DWORD =
         Types.DWORD.withName("negative_or_DWORD");

       



       Types.zero_or_DWORD =
         Types.DWORD.withName("zero_or_DWORD");

       



       Types.zero_or_nothing =
         Types.int.withName("zero_or_nothing");

       Types.SECURITY_ATTRIBUTES =
         Types.void_t.withName("SECURITY_ATTRIBUTES");

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

       Types.FILE_INFORMATION =
         new Type("FILE_INFORMATION",
                  ctypes.StructType("FILE_INFORMATION", [
                    { dwFileAttributes: ctypes.uint32_t },
                    { ftCreationTime:   Types.FILETIME.implementation },
                    { ftLastAccessTime: Types.FILETIME.implementation },
                    { ftLastWriteTime:  Types.FILETIME.implementation },
                    { dwVolumeSerialNumber: ctypes.uint32_t },
                    { nFileSizeHigh:    Types.DWORD.implementation },
                    { nFileSizeLow:     Types.DWORD.implementation },
                    { nNumberOfLinks:   ctypes.uint32_t },
                    { nFileIndex: ctypes.uint64_t }
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

       
       
       let _CloseHandle = WinFile._CloseHandle =
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
                     Types.path,
                       Types.path,
                    Types.bool);

       WinFile.CreateDirectory =
         declareFFI("CreateDirectoryW", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                       Types.jschar.in_ptr,
                    Types.SECURITY_ATTRIBUTES.in_ptr);

       WinFile.CreateFile =
         declareFFI("CreateFileW", ctypes.winapi_abi,
                      Types.maybe_HANDLE,
                        Types.path,
                      Types.DWORD,
                       Types.DWORD,
                    Types.SECURITY_ATTRIBUTES.in_ptr,
                    Types.DWORD,
                       Types.DWORD,
                    Types.HANDLE);

       WinFile.DeleteFile =
         declareFFI("DeleteFileW", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                       Types.path);

       WinFile.FileTimeToSystemTime =
         declareFFI("FileTimeToSystemTime", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                    Types.FILETIME.in_ptr,
                     Types.SystemTime.out_ptr);

       WinFile.FindFirstFile =
         declareFFI("FindFirstFileW", ctypes.winapi_abi,
                     Types.maybe_find_HANDLE,
                    Types.path,
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
                        Types.out_wstring,
                       Types.DWORD,
                    Types.void_t.in_ptr
                   );

       WinFile.GetCurrentDirectory =
         declareFFI("GetCurrentDirectoryW", ctypes.winapi_abi,
                     Types.zero_or_DWORD,
                     Types.DWORD,
                        Types.out_path
                   );

       WinFile.GetFileInformationByHandle =
         declareFFI("GetFileInformationByHandle", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                     Types.HANDLE,
                       Types.FILE_INFORMATION.out_ptr);

       WinFile.MoveFileEx =
         declareFFI("MoveFileExW", ctypes.winapi_abi,
                       Types.zero_or_nothing,
                     Types.path,
                     Types.path,
                        Types.DWORD
                   );

       WinFile.ReadFile =
         declareFFI("ReadFile", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                       Types.HANDLE,
                     Types.voidptr_t,
                     Types.DWORD,
                    Types.DWORD.out_ptr,
                    Types.void_t.inout_ptr 
         );

       WinFile.RemoveDirectory =
         declareFFI("RemoveDirectoryW", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                       Types.path);

       WinFile.SetCurrentDirectory =
         declareFFI("SetCurrentDirectoryW", ctypes.winapi_abi,
                     Types.zero_or_nothing,
                       Types.path
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
                     Types.voidptr_t,
                     Types.DWORD,
                    Types.DWORD.out_ptr,
                    Types.void_t.inout_ptr 
         );
     };
     exports.OS.Win.File._init = init;
   })(this);
}
