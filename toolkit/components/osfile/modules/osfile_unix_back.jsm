



{
  if (typeof Components != "undefined") {
    
    
    
    

    throw new Error("osfile_unix_back.jsm cannot be used from the main thread yet");
  }
  (function(exports) {
     "use strict";
     if (exports.OS && exports.OS.Unix && exports.OS.Unix.File) {
       return; 
     }

     let SharedAll =
       require("resource://gre/modules/osfile/osfile_shared_allthreads.jsm");
     let SysAll =
       require("resource://gre/modules/osfile/osfile_unix_allthreads.jsm");
     let LOG = SharedAll.LOG.bind(SharedAll, "Unix", "back");
     let libc = SysAll.libc;
     let Const = SharedAll.Constants.libc;

     




     
     let init = function init(aDeclareFFI) {
       let declareFFI;
       if (aDeclareFFI) {
         declareFFI = aDeclareFFI.bind(null, libc);
       } else {
         declareFFI = SysAll.declareFFI;
       }
       let declareLazyFFI = SharedAll.declareLazyFFI;

       
       
       
       let Type = Object.create(SysAll.Type);
       let SysFile = exports.OS.Unix.File = { Type: Type };

       


       Type.fd = Type.int.withName("fd");
       Type.fd.importFromC = function importFromC(fd_int) {
         return ctypes.CDataFinalizer(fd_int, SysFile._close);
       };


       



       Type.negativeone_or_fd = Type.fd.withName("negativeone_or_fd");
       Type.negativeone_or_fd.importFromC =
         function importFromC(fd_int) {
           if (fd_int == -1) {
             return -1;
           }
           return ctypes.CDataFinalizer(fd_int, SysFile._close);
         };

       



       Type.negativeone_or_nothing =
         Type.int.withName("negativeone_or_nothing");

       



       Type.negativeone_or_ssize_t =
         Type.ssize_t.withName("negativeone_or_ssize_t");

       


       Type.mode_t =
         Type.intn_t(Const.OSFILE_SIZEOF_MODE_T).withName("mode_t");
       Type.uid_t =
         Type.intn_t(Const.OSFILE_SIZEOF_UID_T).withName("uid_t");
       Type.gid_t =
         Type.intn_t(Const.OSFILE_SIZEOF_GID_T).withName("gid_t");

       


       Type.time_t =
         Type.intn_t(Const.OSFILE_SIZEOF_TIME_T).withName("time_t");

       
       
       
       
       
       
       
       
       
       
       {
         let d_name_extra_size = 0;
         if (Const.OSFILE_SIZEOF_DIRENT_D_NAME < 8) {
           
           
           d_name_extra_size = 256;
         }

         let dirent = new SharedAll.HollowStructure("dirent",
           Const.OSFILE_SIZEOF_DIRENT + d_name_extra_size);
         if (Const.OSFILE_OFFSETOF_DIRENT_D_TYPE != undefined) {
           
           dirent.add_field_at(Const.OSFILE_OFFSETOF_DIRENT_D_TYPE,
             "d_type", ctypes.uint8_t);
         }
         dirent.add_field_at(Const.OSFILE_OFFSETOF_DIRENT_D_NAME,
           "d_name", ctypes.ArrayType(ctypes.char,
             Const.OSFILE_SIZEOF_DIRENT_D_NAME + d_name_extra_size));

         
         Type.dirent = dirent.getType();
       }
       Type.null_or_dirent_ptr =
         new SharedAll.Type("null_of_dirent",
                  Type.dirent.out_ptr.implementation);

       
       
       {
         let stat = new SharedAll.HollowStructure("stat",
           Const.OSFILE_SIZEOF_STAT);
         stat.add_field_at(Const.OSFILE_OFFSETOF_STAT_ST_MODE,
                        "st_mode", Type.mode_t.implementation);
         stat.add_field_at(Const.OSFILE_OFFSETOF_STAT_ST_UID,
                          "st_uid", Type.uid_t.implementation);
         stat.add_field_at(Const.OSFILE_OFFSETOF_STAT_ST_GID,
                          "st_gid", Type.gid_t.implementation);

         
         
         
         
         
         stat.add_field_at(Const.OSFILE_OFFSETOF_STAT_ST_ATIME,
                          "st_atime", Type.time_t.implementation);
         stat.add_field_at(Const.OSFILE_OFFSETOF_STAT_ST_MTIME,
                          "st_mtime", Type.time_t.implementation);
         stat.add_field_at(Const.OSFILE_OFFSETOF_STAT_ST_CTIME,
                          "st_ctime", Type.time_t.implementation);

         
         if ("OSFILE_OFFSETOF_STAT_ST_BIRTHTIME" in Const) {
           stat.add_field_at(Const.OSFILE_OFFSETOF_STAT_ST_BIRTHTIME,
                             "st_birthtime", Type.time_t.implementation);
         }

         stat.add_field_at(Const.OSFILE_OFFSETOF_STAT_ST_SIZE,
                        "st_size", Type.off_t.implementation);
         Type.stat = stat.getType();
       }

       
       if ("OSFILE_SIZEOF_DIR" in Const) {
         
         
         
         let DIR = new SharedAll.HollowStructure(
           "DIR",
           Const.OSFILE_SIZEOF_DIR);

         DIR.add_field_at(
           Const.OSFILE_OFFSETOF_DIR_DD_FD,
           "dd_fd",
           Type.fd.implementation);

         Type.DIR = DIR.getType();
       } else {
         
         Type.DIR =
           new SharedAll.Type("DIR",
             ctypes.StructType("DIR"));
       }

       Type.null_or_DIR_ptr =
         Type.DIR.out_ptr.withName("null_or_DIR*");
       Type.null_or_DIR_ptr.importFromC = function importFromC(dir) {
         if (dir == null || dir.isNull()) {
           return null;
         }
         return ctypes.CDataFinalizer(dir, SysFile._close_dir);
       };

       
       {
         let timeval = new SharedAll.HollowStructure(
           "timeval",
           Const.OSFILE_SIZEOF_TIMEVAL);
         timeval.add_field_at(
           Const.OSFILE_OFFSETOF_TIMEVAL_TV_SEC,
           "tv_sec",
           Type.long.implementation);
         timeval.add_field_at(
           Const.OSFILE_OFFSETOF_TIMEVAL_TV_USEC,
           "tv_usec",
           Type.long.implementation);
         Type.timeval = timeval.getType();
         Type.timevals = new SharedAll.Type("two timevals",
           ctypes.ArrayType(Type.timeval.implementation, 2));
       }

       

       
       libc.declareLazy(SysFile, "_close",
                        "close", ctypes.default_abi,
                        ctypes.int,
                             ctypes.int);

       SysFile.close = function close(fd) {
         
         return fd.dispose();
       };

       libc.declareLazy(SysFile, "_close_dir",
                        "closedir", ctypes.default_abi,
                        ctypes.int,
                           Type.DIR.in_ptr.implementation);

       SysFile.closedir = function closedir(fd) {
         
         return fd.dispose();
       };

       {
         
         
         let default_lib = new SharedAll.Library("default_lib",
                                                 "a.out");
         try {
           
           
           

           default_lib.declareLazy(SysFile, "free",
             "free", ctypes.default_abi,
              ctypes.void_t,
                 ctypes.voidptr_t);

         } catch (ex) {
           
           

           libc.declareLazy(SysFile, "free",
             "free", ctypes.default_abi,
              ctypes.void_t,
                 ctypes.voidptr_t);
         }
      }


       
       libc.declareLazyFFI(SysFile,  "access",
                           "access", ctypes.default_abi,
                            Type.negativeone_or_nothing,
                              Type.path,
                              Type.int);

       libc.declareLazyFFI(SysFile,  "chdir",
                           "chdir", ctypes.default_abi,
                            Type.negativeone_or_nothing,
                              Type.path);

       libc.declareLazyFFI(SysFile,  "chmod",
                           "chmod", ctypes.default_abi,
                            Type.negativeone_or_nothing,
                              Type.path,
                              Type.mode_t);

       libc.declareLazyFFI(SysFile,  "chown",
                           "chown", ctypes.default_abi,
                            Type.negativeone_or_nothing,
                              Type.path,
                               Type.uid_t,
                               Type.gid_t);

       libc.declareLazyFFI(SysFile,  "copyfile",
                           "copyfile", ctypes.default_abi,
                            Type.negativeone_or_nothing,
                            Type.path,
                              Type.path,
                             Type.void_t.in_ptr, 
                             Type.uint32_t);

       libc.declareLazyFFI(SysFile,  "dup",
                           "dup", ctypes.default_abi,
                            Type.negativeone_or_fd,
                                Type.fd);

       if ("OSFILE_SIZEOF_DIR" in Const) {
         
         SysFile.dirfd =
           function dirfd(DIRp) {
             return Type.DIR.in_ptr.implementation(DIRp).contents.dd_fd;
           };
       } else {
         
         libc.declareLazyFFI(SysFile,  "dirfd",
                             "dirfd", ctypes.default_abi,
                              Type.negativeone_or_fd,
                                 Type.DIR.in_ptr);
       }

       libc.declareLazyFFI(SysFile,  "chdir",
                           "chdir", ctypes.default_abi,
                            Type.negativeone_or_nothing,
                              Type.path);

       libc.declareLazyFFI(SysFile,  "fchdir",
                           "fchdir", ctypes.default_abi,
                            Type.negativeone_or_nothing,
                                Type.fd);

       libc.declareLazyFFI(SysFile,  "fchown",
                           "fchown", ctypes.default_abi,
                            Type.negativeone_or_nothing,
                                Type.fd,
                             Type.uid_t,
                             Type.gid_t);

       libc.declareLazyFFI(SysFile,  "fsync",
                           "fsync", ctypes.default_abi,
                            Type.negativeone_or_nothing,
                                Type.fd);

       libc.declareLazyFFI(SysFile,  "getcwd",
                           "getcwd", ctypes.default_abi,
                            Type.out_path,
                               Type.out_path,
                              Type.size_t);

       libc.declareLazyFFI(SysFile,  "getwd",
                           "getwd", ctypes.default_abi,
                            Type.out_path,
                               Type.out_path);

       
       

       
       libc.declareLazyFFI(SysFile,  "get_current_dir_name",
                           "get_current_dir_name", ctypes.default_abi,
                            Type.out_path.releaseWithLazy(() =>
                             SysFile.free
                           ));

       
       libc.declareLazyFFI(SysFile,  "getwd_auto",
                           "getwd", ctypes.default_abi,
                            Type.out_path.releaseWithLazy(() =>
                             SysFile.free
                           ),
                               Type.void_t.out_ptr);

       libc.declareLazyFFI(SysFile,  "fdatasync",
                           "fdatasync", ctypes.default_abi,
                            Type.negativeone_or_nothing,
                                Type.fd); 

       libc.declareLazyFFI(SysFile,  "ftruncate",
                           "ftruncate", ctypes.default_abi,
                            Type.negativeone_or_nothing,
                                Type.fd,
                            Type.off_t);


       libc.declareLazyFFI(SysFile,  "lchown",
                           "lchown", ctypes.default_abi,
                            Type.negativeone_or_nothing,
                              Type.path,
                             Type.uid_t,
                             Type.gid_t);

       libc.declareLazyFFI(SysFile,  "link",
                           "link", ctypes.default_abi,
                            Type.negativeone_or_nothing,
                            Type.path,
                              Type.path);

       libc.declareLazyFFI(SysFile,  "lseek",
                           "lseek", ctypes.default_abi,
                            Type.off_t,
                                Type.fd,
                            Type.off_t,
                            Type.int);

       libc.declareLazyFFI(SysFile,  "mkdir",
                           "mkdir", ctypes.default_abi,
                            Type.int,
                            Type.path,
                            Type.int);

       libc.declareLazyFFI(SysFile,  "mkstemp",
                           "mkstemp", ctypes.default_abi,
                              Type.fd,
                            Type.out_path);

       libc.declareLazyFFI(SysFile,  "open",
                           "open", ctypes.default_abi,
                            Type.negativeone_or_fd,
                              Type.path,
                            Type.int,
                              Type.int);

       if (OS.Constants.Sys.Name == "NetBSD") {
          libc.declareLazyFFI(SysFile,  "opendir",
                              "__opendir30", ctypes.default_abi,
                    Type.null_or_DIR_ptr,
                      Type.path);
       } else {
          libc.declareLazyFFI(SysFile,  "opendir",
                              "opendir", ctypes.default_abi,
                     Type.null_or_DIR_ptr,
                       Type.path);
       }

       libc.declareLazyFFI(SysFile,  "pread",
                               "pread", ctypes.default_abi,
                     Type.negativeone_or_ssize_t,
                         Type.fd,
                        Type.void_t.out_ptr,
                     Type.size_t,
                     Type.off_t);

       libc.declareLazyFFI(SysFile,  "pwrite",
                               "pwrite", ctypes.default_abi,
                     Type.negativeone_or_ssize_t,
                         Type.fd,
                        Type.void_t.in_ptr,
                     Type.size_t,
                     Type.off_t);

       libc.declareLazyFFI(SysFile,  "read",
                              "read", ctypes.default_abi,
                    Type.negativeone_or_ssize_t,
                        Type.fd,
                       Type.void_t.out_ptr,
                    Type.size_t);

       libc.declareLazyFFI(SysFile,  "posix_fadvise",
                               "posix_fadvise", ctypes.default_abi,
                     Type.int,
                         Type.fd,
                     Type.off_t,
                        Type.off_t,
                     Type.int);

       if (Const._DARWIN_FEATURE_64_BIT_INODE) {
         
         
         
         
         libc.declareLazyFFI(SysFile,  "readdir",
                                "readdir$INODE64", ctypes.default_abi,
                      Type.null_or_dirent_ptr,
                         Type.DIR.in_ptr); 
       } else if (OS.Constants.Sys.Name == "NetBSD") {
         libc.declareLazyFFI(SysFile,  "readdir",
                                "__readdir30", ctypes.default_abi,
                      Type.null_or_dirent_ptr,
                         Type.DIR.in_ptr); 
       } else {
         libc.declareLazyFFI(SysFile,  "readdir",
                                "readdir", ctypes.default_abi,
                      Type.null_or_dirent_ptr,
                         Type.DIR.in_ptr); 
       }

       libc.declareLazyFFI(SysFile,  "rename",
                               "rename", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                        Type.path,
                        Type.path);

       libc.declareLazyFFI(SysFile,  "rmdir",
                               "rmdir", ctypes.default_abi,
                     Type.int,
                       Type.path);

       libc.declareLazyFFI(SysFile,  "splice",
                               "splice", ctypes.default_abi,
                     Type.long,
                      Type.fd,
                     Type.off_t.in_ptr,
                     Type.fd,
                    Type.off_t.in_ptr,
                        Type.size_t,
                      Type.unsigned_int); 

       libc.declareLazyFFI(SysFile,  "symlink",
                               "symlink", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                     Type.path,
                       Type.path);

       libc.declareLazyFFI(SysFile,  "truncate",
                              "truncate", ctypes.default_abi,
                    Type.negativeone_or_nothing,
                      Type.path,
                     Type.off_t);

       libc.declareLazyFFI(SysFile,  "unlink",
                           "unlink", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                     Type.path);

       libc.declareLazyFFI(SysFile,  "write",
                           "write", ctypes.default_abi,
                     Type.negativeone_or_ssize_t,
                         Type.fd,
                        Type.void_t.in_ptr,
                     Type.size_t);

       

       
       
       if (Const._DARWIN_FEATURE_64_BIT_INODE) {
         
         libc.declareLazyFFI(SysFile,  "stat",
                             "stat$INODE64", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                         Type.path,
                          Type.stat.out_ptr
                     );
         libc.declareLazyFFI(SysFile,  "lstat",
                             "lstat$INODE64", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                         Type.path,
                          Type.stat.out_ptr
                     );
         libc.declareLazyFFI(SysFile,  "fstat",
                             "fstat$INODE64", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                         Type.fd,
                          Type.stat.out_ptr
                     );
       } else if (Const._STAT_VER != undefined) {
         const ver = Const._STAT_VER;
         let xstat_name, lxstat_name, fxstat_name;
         if (OS.Constants.Sys.Name == "SunOS") {
           
           xstat_name = "_xstat";
           lxstat_name = "_lxstat";
           fxstat_name = "_fxstat";
         } else {
           
           xstat_name = "__xstat";
           lxstat_name = "__lxstat";
           fxstat_name = "__fxstat";
         }

         let Stat = {};
         libc.declareLazyFFI(Stat,  "xstat",
                             xstat_name, ctypes.default_abi,
                          Type.negativeone_or_nothing,
                       Type.int,
                            Type.path,
                             Type.stat.out_ptr);
         libc.declareLazyFFI(Stat,  "lxstat",
                             lxstat_name, ctypes.default_abi,
                          Type.negativeone_or_nothing,
                       Type.int,
                            Type.path,
                             Type.stat.out_ptr);
         libc.declareLazyFFI(Stat, "fxstat",
                             fxstat_name, ctypes.default_abi,
                          Type.negativeone_or_nothing,
                       Type.int,
                              Type.fd,
                             Type.stat.out_ptr);

         
         SysFile.stat = function stat(path, buf) {
           return Stat.xstat(ver, path, buf);
         };

         SysFile.lstat = function lstat(path, buf) {
           return Stat.lxstat(ver, path, buf);
         };

         SysFile.fstat = function fstat(fd, buf) {
           return Stat.fxstat(ver, fd, buf);
         };
       } else if (OS.Constants.Sys.Name == "NetBSD") {
         
         libc.declareLazyFFI(SysFile,  "stat",
                             "__stat50", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                         Type.path,
                          Type.stat.out_ptr
                     );
         libc.declareLazyFFI(SysFile,  "lstat",
                             "__lstat50", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                         Type.path,
                          Type.stat.out_ptr
                     );
         libc.declareLazyFFI(SysFile,  "fstat",
                             "__fstat50", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                           Type.fd,
                          Type.stat.out_ptr
                     );
       } else {
         
         libc.declareLazyFFI(SysFile,  "stat",
                             "stat", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                         Type.path,
                          Type.stat.out_ptr
                     );
         libc.declareLazyFFI(SysFile,  "lstat",
                             "lstat", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                         Type.path,
                          Type.stat.out_ptr
                     );
         libc.declareLazyFFI(SysFile,  "fstat",
                             "fstat", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                           Type.fd,
                          Type.stat.out_ptr
                     );
       }

       
       

       let Pipe = {};
       libc.declareLazyFFI(Pipe, "_pipe",
         "pipe", ctypes.default_abi,
          Type.negativeone_or_nothing,
             new SharedAll.Type("two file descriptors",
             ctypes.ArrayType(ctypes.int, 2)));

       
       let _pipebuf = new (ctypes.ArrayType(ctypes.int, 2))();

       SysFile.pipe = function pipe(array) {
         let result = Pipe._pipe(_pipebuf);
         if (result == -1) {
           return result;
         }
         array[0] = ctypes.CDataFinalizer(_pipebuf[0], SysFile._close);
         array[1] = ctypes.CDataFinalizer(_pipebuf[1], SysFile._close);
         return result;
       };

       if (OS.Constants.Sys.Name == "NetBSD") {
           libc.declareLazyFFI(SysFile, "utimes",
                               "__utimes50", ctypes.default_abi,
                           Type.negativeone_or_nothing,
                             Type.path,
                       Type.timevals.out_ptr
                      );
       } else {
           libc.declareLazyFFI(SysFile, "utimes",
                               "utimes", ctypes.default_abi,
                           Type.negativeone_or_nothing,
                             Type.path,
                       Type.timevals.out_ptr
                      );
       }
       if (OS.Constants.Sys.Name == "NetBSD") {
           libc.declareLazyFFI(SysFile, "futimes",
                               "__futimes50", ctypes.default_abi,
                           Type.negativeone_or_nothing,
                               Type.fd,
                       Type.timevals.out_ptr
                      );
       } else {
           libc.declareLazyFFI(SysFile, "futimes",
                               "futimes", ctypes.default_abi,
                           Type.negativeone_or_nothing,
                               Type.fd,
                       Type.timevals.out_ptr
                      );
       }
     };

     exports.OS.Unix = {
       File: {
         _init: init
       }
     };
   })(this);
}
