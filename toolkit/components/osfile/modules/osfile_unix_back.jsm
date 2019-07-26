



{
  if (typeof Components != "undefined") {
    
    
    
    

    throw new Error("osfile_unix_back.jsm cannot be used from the main thread yet");
  }
  (function(exports) {
     "use strict";
     if (exports.OS && exports.OS.Unix && exports.OS.Unix.File) {
       return; 
     }

     exports.OS = require("resource://gre/modules/osfile/osfile_shared_allthreads.jsm").OS;

     exports.OS.Unix.File = {};

     let LOG = exports.OS.Shared.LOG.bind(OS.Shared, "Unix", "back");
     let libc = exports.OS.Shared.Unix.libc;

     




     
     let init = function init(aDeclareFFI) {
       let declareFFI;
       if (aDeclareFFI) {
         declareFFI = aDeclareFFI.bind(null, libc);
       } else {
         declareFFI = exports.OS.Shared.Unix.declareFFI;
       }

       
       let OSUnix = exports.OS.Unix;
       let UnixFile = exports.OS.Unix.File;
       if (!exports.OS.Types) {
         exports.OS.Types = {};
       }
       let Type = exports.OS.Shared.Type;
       let Types = Type;

       
       
       

       


       Types.fd = Type.int.withName("fd");
       Types.fd.importFromC = function importFromC(fd_int) {
         return ctypes.CDataFinalizer(fd_int, _close);
       };


       



       Types.negativeone_or_fd = Types.fd.withName("negativeone_or_fd");
       Types.negativeone_or_fd.importFromC =
         function importFromC(fd_int) {
           if (fd_int == -1) {
             return -1;
           }
           return ctypes.CDataFinalizer(fd_int, _close);
         };

       



       Types.negativeone_or_nothing =
         Types.int.withName("negativeone_or_nothing");

       



       Types.negativeone_or_ssize_t =
         Types.ssize_t.withName("negativeone_or_ssize_t");

       


       Types.mode_t =
         Types.intn_t(OS.Constants.libc.OSFILE_SIZEOF_MODE_T).withName("mode_t");
       Types.uid_t =
         Types.intn_t(OS.Constants.libc.OSFILE_SIZEOF_UID_T).withName("uid_t");
       Types.gid_t =
         Types.intn_t(OS.Constants.libc.OSFILE_SIZEOF_GID_T).withName("gid_t");

       


       Types.time_t =
         Types.intn_t(OS.Constants.libc.OSFILE_SIZEOF_TIME_T).withName("time_t");

       
       
       
       
       
       
       
       
       
       
       {
         let d_name_extra_size = 0;
         if (OS.Constants.libc.OSFILE_SIZEOF_DIRENT_D_NAME < 8) {
           
           
           d_name_extra_size = 256;
         }

         let dirent = new OS.Shared.HollowStructure("dirent",
           OS.Constants.libc.OSFILE_SIZEOF_DIRENT + d_name_extra_size);
         if (OS.Constants.libc.OSFILE_OFFSETOF_DIRENT_D_TYPE != undefined) {
           
           dirent.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_DIRENT_D_TYPE,
             "d_type", ctypes.uint8_t);
         }
         dirent.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_DIRENT_D_NAME,
           "d_name", ctypes.ArrayType(ctypes.char,
             OS.Constants.libc.OSFILE_SIZEOF_DIRENT_D_NAME + d_name_extra_size));

         
         Types.dirent = dirent.getType();
       }
       Types.null_or_dirent_ptr =
         new Type("null_of_dirent",
                  Types.dirent.out_ptr.implementation);

       
       
       {
         let stat = new OS.Shared.HollowStructure("stat",
           OS.Constants.libc.OSFILE_SIZEOF_STAT);
         stat.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_STAT_ST_MODE,
                        "st_mode", Types.mode_t.implementation);
         stat.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_STAT_ST_UID,
                          "st_uid", Types.uid_t.implementation);
         stat.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_STAT_ST_GID,
                          "st_gid", Types.gid_t.implementation);

         
         
         
         
         
         stat.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_STAT_ST_ATIME,
                          "st_atime", Types.time_t.implementation);
         stat.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_STAT_ST_MTIME,
                          "st_mtime", Types.time_t.implementation);
         stat.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_STAT_ST_CTIME,
                          "st_ctime", Types.time_t.implementation);

         
         if ("OSFILE_OFFSETOF_STAT_ST_BIRTHTIME" in OS.Constants.libc) {
           stat.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_STAT_ST_BIRTHTIME,
                             "st_birthtime", Types.time_t.implementation);
         }

         stat.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_STAT_ST_SIZE,
                        "st_size", Types.size_t.implementation);
         Types.stat = stat.getType();
       }

       
       if ("OSFILE_SIZEOF_DIR" in OS.Constants.libc) {
         
         
         
         let DIR = new OS.Shared.HollowStructure(
           "DIR",
           OS.Constants.libc.OSFILE_SIZEOF_DIR);

         DIR.add_field_at(
           OS.Constants.libc.OSFILE_OFFSETOF_DIR_DD_FD,
           "dd_fd",
           Types.fd.implementation);

         Types.DIR = DIR.getType();
       } else {
         
         Types.DIR =
           new Type("DIR",
             ctypes.StructType("DIR"));
       }

       Types.null_or_DIR_ptr =
         Types.DIR.out_ptr.withName("null_or_DIR*");
       Types.null_or_DIR_ptr.importFromC = function importFromC(dir) {
         if (dir == null || dir.isNull()) {
           return null;
         }
         return ctypes.CDataFinalizer(dir, _close_dir);
       };

       

       
       let _close = UnixFile._close =
         libc.declare("close", ctypes.default_abi,
                        ctypes.int,
                             ctypes.int);

       UnixFile.close = function close(fd) {
         
         return fd.dispose();
       };

       let _close_dir =
         libc.declare("closedir", ctypes.default_abi,
                        ctypes.int,
                           Types.DIR.in_ptr.implementation);

       UnixFile.closedir = function closedir(fd) {
         
         return fd.dispose();
       };

       {
         
         
         let default_lib = libc;
         try {
           
           
           
           default_lib = ctypes.open("a.out");

           UnixFile.free =
             default_lib.declare("free", ctypes.default_abi,
              ctypes.void_t,
                 ctypes.voidptr_t);

         } catch (ex) {
           
           

           UnixFile.free =
             libc.declare("free", ctypes.default_abi,
              ctypes.void_t,
                 ctypes.voidptr_t);
         }
      }


       
       UnixFile.access =
         declareFFI("access", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                       Types.path,
                       Types.int);

       UnixFile.chdir =
         declareFFI("chdir", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                       Types.path);

       UnixFile.chmod =
         declareFFI("chmod", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                       Types.path,
                       Types.mode_t);

       UnixFile.chown =
         declareFFI("chown", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                       Types.path,
                        Types.uid_t,
                        Types.gid_t);

       UnixFile.copyfile =
         declareFFI("copyfile", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                     Types.path,
                       Types.path,
                      Types.void_t.in_ptr, 
                      Types.uint32_t);

       UnixFile.dup =
         declareFFI("dup", ctypes.default_abi,
                     Types.negativeone_or_fd,
                         Types.fd);

       if ("OSFILE_SIZEOF_DIR" in OS.Constants.libc) {
         
         UnixFile.dirfd =
           function dirfd(DIRp) {
             return Types.DIR.in_ptr.implementation(DIRp).contents.dd_fd;
           };
       } else {
         
         UnixFile.dirfd =
           declareFFI("dirfd", ctypes.default_abi,
                       Types.negativeone_or_fd,
                          Types.DIR.in_ptr);
       }

       UnixFile.chdir =
         declareFFI("chdir", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                       Types.path);

       UnixFile.fchdir =
         declareFFI("fchdir", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                         Types.fd);

       UnixFile.fchown =
         declareFFI("fchown", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                         Types.fd,
                      Types.uid_t,
                      Types.gid_t);

       UnixFile.fsync =
         declareFFI("fsync", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                         Types.fd);

       UnixFile.getcwd =
         declareFFI("getcwd", ctypes.default_abi,
                     Types.out_path,
                        Types.out_path,
                       Types.size_t);

       UnixFile.getwd =
         declareFFI("getwd", ctypes.default_abi,
                     Types.out_path,
                        Types.out_path);

       
       

       
       UnixFile.get_current_dir_name =
         declareFFI("get_current_dir_name", ctypes.default_abi,
                     Types.out_path.releaseWith(UnixFile.free));

       
       UnixFile.getwd_auto =
         declareFFI("getwd", ctypes.default_abi,
                     Types.out_path.releaseWith(UnixFile.free),
                        Types.void_t.out_ptr);

       UnixFile.fdatasync =
         declareFFI("fdatasync", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                         Types.fd); 

       UnixFile.ftruncate =
         declareFFI("ftruncate", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                         Types.fd,
                     Types.off_t);

       if (OS.Constants.libc._DARWIN_FEATURE_64_BIT_INODE) {
         UnixFile.fstat =
           declareFFI("fstat$INODE64", ctypes.default_abi,
                       Types.negativeone_or_nothing,
                         Types.fd,
                          Types.stat.out_ptr
                     );
       } else {
         UnixFile.fstat =
           declareFFI("fstat", ctypes.default_abi,
                       Types.negativeone_or_nothing,
                         Types.fd,
                          Types.stat.out_ptr
                     );
       }

       UnixFile.lchown =
         declareFFI("lchown", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                       Types.path,
                      Types.uid_t,
                      Types.gid_t);

       UnixFile.link =
         declareFFI("link", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                     Types.path,
                       Types.path);

       UnixFile.lseek =
         declareFFI("lseek", ctypes.default_abi,
                     Types.off_t,
                         Types.fd,
                     Types.off_t,
                     Types.int);

       UnixFile.mkdir =
         declareFFI("mkdir", ctypes.default_abi,
                     Types.int,
                     Types.path,
                     Types.int);

       UnixFile.mkstemp =
         declareFFI("mkstemp", ctypes.default_abi,
                     Types.fd,
                    Types.out_path);

       UnixFile.open =
         declareFFI("open", ctypes.default_abi,
                    Types.negativeone_or_fd,
                      Types.path,
                    Types.int,
                      Types.int);

       UnixFile.opendir =
         declareFFI("opendir", ctypes.default_abi,
                     Types.null_or_DIR_ptr,
                       Types.path);

       UnixFile.pread =
         declareFFI("pread", ctypes.default_abi,
                     Types.negativeone_or_ssize_t,
                         Types.fd,
                        Types.void_t.out_ptr,
                     Types.size_t,
                     Types.off_t);

       UnixFile.pwrite =
         declareFFI("pwrite", ctypes.default_abi,
                     Types.negativeone_or_ssize_t,
                         Types.fd,
                        Types.void_t.in_ptr,
                     Types.size_t,
                     Types.off_t);

       UnixFile.read =
         declareFFI("read", ctypes.default_abi,
                    Types.negativeone_or_ssize_t,
                        Types.fd,
                       Types.void_t.out_ptr,
                    Types.size_t);

       if (OS.Constants.libc._DARWIN_FEATURE_64_BIT_INODE) {
         
         
         
         
         UnixFile.readdir =
           declareFFI("readdir$INODE64", ctypes.default_abi,
                     Types.null_or_dirent_ptr,
                         Types.DIR.in_ptr); 
       } else {
         UnixFile.readdir =
           declareFFI("readdir", ctypes.default_abi,
                      Types.null_or_dirent_ptr,
                         Types.DIR.in_ptr); 
       }

       UnixFile.rename =
         declareFFI("rename", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                        Types.path,
                        Types.path);

       UnixFile.rmdir =
         declareFFI("rmdir", ctypes.default_abi,
                     Types.int,
                       Types.path);

       UnixFile.splice =
         declareFFI("splice", ctypes.default_abi,
                     Types.long,
                      Types.fd,
                     Types.off_t.in_ptr,
                     Types.fd,
                    Types.off_t.in_ptr,
                        Types.size_t,
                      Types.unsigned_int); 

       UnixFile.symlink =
         declareFFI("symlink", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                     Types.path,
                       Types.path);

       UnixFile.truncate =
         declareFFI("truncate", ctypes.default_abi,
                    Types.negativeone_or_nothing,
                      Types.path,
                     Types.off_t);

       UnixFile.unlink =
         declareFFI("unlink", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                     Types.path);

       UnixFile.write =
         declareFFI("write", ctypes.default_abi,
                     Types.negativeone_or_ssize_t,
                         Types.fd,
                        Types.void_t.in_ptr,
                     Types.size_t);

       

       
       
       if (OS.Constants.libc._DARWIN_FEATURE_64_BIT_INODE) {
         
         UnixFile.stat =
           declareFFI("stat$INODE64", ctypes.default_abi,
                       Types.negativeone_or_nothing,
                         Types.path,
                          Types.stat.out_ptr
                     );
         UnixFile.lstat =
           declareFFI("lstat$INODE64", ctypes.default_abi,
                       Types.negativeone_or_nothing,
                         Types.path,
                          Types.stat.out_ptr
                     );
         UnixFile.fstat =
           declareFFI("fstat$INODE64", ctypes.default_abi,
                       Types.negativeone_or_nothing,
                         Types.fd,
                          Types.stat.out_ptr
                     );
       } else if (OS.Constants.libc._STAT_VER != undefined) {
         const ver = OS.Constants.libc._STAT_VER;
         let xstat_name, lxstat_name, fxstat_name
         if (OS.Constants.Sys.Name == "SunOS") {
           
           xstat_name = "_xstat";
           lxstat_name = "_lxstat";
           fxstat_name = "_fxstat";
         } else {
           
           xstat_name = "__xstat";
           lxstat_name = "__lxstat";
           fxstat_name = "__fxstat";
         }

         let xstat =
           declareFFI(xstat_name, ctypes.default_abi,
                          Types.negativeone_or_nothing,
                       Types.int,
                            Types.path,
                             Types.stat.out_ptr);
         let lxstat =
           declareFFI(lxstat_name, ctypes.default_abi,
                          Types.negativeone_or_nothing,
                       Types.int,
                            Types.path,
                             Types.stat.out_ptr);
         let fxstat =
           declareFFI(fxstat_name, ctypes.default_abi,
                          Types.negativeone_or_nothing,
                       Types.int,
                              Types.fd,
                             Types.stat.out_ptr);

         UnixFile.stat = function stat(path, buf) {
           return xstat(ver, path, buf);
         };
         UnixFile.lstat = function stat(path, buf) {
           return lxstat(ver, path, buf);
         };
         UnixFile.fstat = function stat(fd, buf) {
           return fxstat(ver, fd, buf);
         };
       } else {
         
         UnixFile.stat =
           declareFFI("stat", ctypes.default_abi,
                       Types.negativeone_or_nothing,
                         Types.path,
                          Types.stat.out_ptr
                     );
         UnixFile.lstat =
           declareFFI("lstat", ctypes.default_abi,
                       Types.negativeone_or_nothing,
                         Types.path,
                          Types.stat.out_ptr
                     );
         UnixFile.fstat =
           declareFFI("fstat", ctypes.default_abi,
                       Types.negativeone_or_nothing,
                           Types.fd,
                          Types.stat.out_ptr
                     );
       }

       
       

       let _pipe =
         declareFFI("pipe", ctypes.default_abi,
            Types.negativeone_or_nothing,
               new Type("two file descriptors",
             ctypes.ArrayType(ctypes.int, 2)));

       
       let _pipebuf = new (ctypes.ArrayType(ctypes.int, 2))();

       UnixFile.pipe = function pipe(array) {
         let result = _pipe(_pipebuf);
         if (result == -1) {
           return result;
         }
         array[0] = ctypes.CDataFinalizer(_pipebuf[0], _close);
         array[1] = ctypes.CDataFinalizer(_pipebuf[1], _close);
         return result;
       };
     };
     exports.OS.Unix.File._init = init;
   })(this);
}
