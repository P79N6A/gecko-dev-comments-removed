



{
  if (typeof Components != "undefined") {
    
    
    
    

    throw new Error("osfile_unix_back.jsm cannot be used from the main thread yet");
  }
  (function(exports) {
     "use strict";
     if (exports.OS && exports.OS.Unix && exports.OS.Unix.File) {
       return; 
     }

     let SharedAll = require("resource://gre/modules/osfile/osfile_shared_allthreads.jsm");
     let SysAll = require("resource://gre/modules/osfile/osfile_unix_allthreads.jsm");
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

       
       
       
       let Type = Object.create(SysAll.Type);
       let SysFile = exports.OS.Unix.File = { Type: Type };

       


       Type.fd = Type.int.withName("fd");
       Type.fd.importFromC = function importFromC(fd_int) {
         return ctypes.CDataFinalizer(fd_int, _close);
       };


       



       Type.negativeone_or_fd = Type.fd.withName("negativeone_or_fd");
       Type.negativeone_or_fd.importFromC =
         function importFromC(fd_int) {
           if (fd_int == -1) {
             return -1;
           }
           return ctypes.CDataFinalizer(fd_int, _close);
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
         Type.dirent.out_ptr.withName("null_or_dirent");

       
       
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
                        "st_size", Type.size_t.implementation);
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
         return ctypes.CDataFinalizer(dir, _close_dir);
       };

       

       
       let _close = SysFile._close =
         libc.declare("close", ctypes.default_abi,
                        ctypes.int,
                             ctypes.int);

       SysFile.close = function close(fd) {
         
         return fd.dispose();
       };

       let _close_dir =
         libc.declare("closedir", ctypes.default_abi,
                        ctypes.int,
                           Type.DIR.in_ptr.implementation);

       SysFile.closedir = function closedir(fd) {
         
         return fd.dispose();
       };

       {
         
         
         let default_lib = libc;
         try {
           
           
           
           default_lib = ctypes.open("a.out");

           SysFile.free =
             default_lib.declare("free", ctypes.default_abi,
              ctypes.void_t,
                 ctypes.voidptr_t);

         } catch (ex) {
           
           

           SysFile.free =
             libc.declare("free", ctypes.default_abi,
              ctypes.void_t,
                 ctypes.voidptr_t);
         }
      }


       
       SysFile.access =
         declareFFI("access", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                       Type.path,
                       Type.int);

       SysFile.chdir =
         declareFFI("chdir", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                       Type.path);

       SysFile.chmod =
         declareFFI("chmod", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                       Type.path,
                       Type.mode_t);

       SysFile.chown =
         declareFFI("chown", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                       Type.path,
                        Type.uid_t,
                        Type.gid_t);

       SysFile.copyfile =
         declareFFI("copyfile", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                     Type.path,
                       Type.path,
                      Type.void_t.in_ptr, 
                      Type.uint32_t);

       SysFile.dup =
         declareFFI("dup", ctypes.default_abi,
                     Type.negativeone_or_fd,
                         Type.fd);

       if ("OSFILE_SIZEOF_DIR" in OS.Constants.libc) {
         
         SysFile.dirfd =
           function dirfd(DIRp) {
             return Type.DIR.in_ptr.implementation(DIRp).contents.dd_fd;
           };
       } else {
         
         SysFile.dirfd =
           declareFFI("dirfd", ctypes.default_abi,
                       Type.negativeone_or_fd,
                          Type.DIR.in_ptr);
       }

       SysFile.chdir =
         declareFFI("chdir", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                       Type.path);

       SysFile.fchdir =
         declareFFI("fchdir", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                         Type.fd);

       SysFile.fchown =
         declareFFI("fchown", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                         Type.fd,
                      Type.uid_t,
                      Type.gid_t);

       SysFile.fsync =
         declareFFI("fsync", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                         Type.fd);

       SysFile.getcwd =
         declareFFI("getcwd", ctypes.default_abi,
                     Type.out_path,
                        Type.out_path,
                       Type.size_t);

       SysFile.getwd =
         declareFFI("getwd", ctypes.default_abi,
                     Type.out_path,
                        Type.out_path);

       
       

       
       SysFile.get_current_dir_name =
         declareFFI("get_current_dir_name", ctypes.default_abi,
                     Type.out_path.releaseWith(SysFile.free));

       
       SysFile.getwd_auto =
         declareFFI("getwd", ctypes.default_abi,
                     Type.out_path.releaseWith(SysFile.free),
                        Type.void_t.out_ptr);

       SysFile.fdatasync =
         declareFFI("fdatasync", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                         Type.fd); 

       SysFile.ftruncate =
         declareFFI("ftruncate", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                         Type.fd,
                     Type.off_t);

       if (OS.Constants.libc._DARWIN_FEATURE_64_BIT_INODE) {
         SysFile.fstat =
           declareFFI("fstat$INODE64", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                         Type.fd,
                          Type.stat.out_ptr
                     );
       } else {
         SysFile.fstat =
           declareFFI("fstat", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                         Type.fd,
                          Type.stat.out_ptr
                     );
       }

       SysFile.lchown =
         declareFFI("lchown", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                       Type.path,
                      Type.uid_t,
                      Type.gid_t);

       SysFile.link =
         declareFFI("link", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                     Type.path,
                       Type.path);

       SysFile.lseek =
         declareFFI("lseek", ctypes.default_abi,
                     Type.off_t,
                         Type.fd,
                     Type.off_t,
                     Type.int);

       SysFile.mkdir =
         declareFFI("mkdir", ctypes.default_abi,
                     Type.int,
                     Type.path,
                     Type.int);

       SysFile.mkstemp =
         declareFFI("mkstemp", ctypes.default_abi,
                     Type.fd,
                    Type.out_path);

       SysFile.open =
         declareFFI("open", ctypes.default_abi,
                    Type.negativeone_or_fd,
                      Type.path,
                    Type.int,
                      Type.int);

       SysFile.opendir =
         declareFFI("opendir", ctypes.default_abi,
                     Type.null_or_DIR_ptr,
                       Type.path);

       SysFile.pread =
         declareFFI("pread", ctypes.default_abi,
                     Type.negativeone_or_ssize_t,
                         Type.fd,
                        Type.void_t.out_ptr,
                     Type.size_t,
                     Type.off_t);

       SysFile.pwrite =
         declareFFI("pwrite", ctypes.default_abi,
                     Type.negativeone_or_ssize_t,
                         Type.fd,
                        Type.void_t.in_ptr,
                     Type.size_t,
                     Type.off_t);

       SysFile.read =
         declareFFI("read", ctypes.default_abi,
                    Type.negativeone_or_ssize_t,
                        Type.fd,
                       Type.void_t.out_ptr,
                    Type.size_t);

       if (OS.Constants.libc._DARWIN_FEATURE_64_BIT_INODE) {
         
         
         
         
         SysFile.readdir =
           declareFFI("readdir$INODE64", ctypes.default_abi,
                     Type.null_or_dirent_ptr,
                         Type.DIR.in_ptr); 
       } else {
         SysFile.readdir =
           declareFFI("readdir", ctypes.default_abi,
                      Type.null_or_dirent_ptr,
                         Type.DIR.in_ptr); 
       }

       SysFile.rename =
         declareFFI("rename", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                        Type.path,
                        Type.path);

       SysFile.rmdir =
         declareFFI("rmdir", ctypes.default_abi,
                     Type.int,
                       Type.path);

       SysFile.splice =
         declareFFI("splice", ctypes.default_abi,
                     Type.long,
                      Type.fd,
                     Type.off_t.in_ptr,
                     Type.fd,
                    Type.off_t.in_ptr,
                        Type.size_t,
                      Type.unsigned_int); 

       SysFile.symlink =
         declareFFI("symlink", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                     Type.path,
                       Type.path);

       SysFile.truncate =
         declareFFI("truncate", ctypes.default_abi,
                    Type.negativeone_or_nothing,
                      Type.path,
                     Type.off_t);

       SysFile.unlink =
         declareFFI("unlink", ctypes.default_abi,
                     Type.negativeone_or_nothing,
                     Type.path);

       SysFile.write =
         declareFFI("write", ctypes.default_abi,
                     Type.negativeone_or_ssize_t,
                         Type.fd,
                        Type.void_t.in_ptr,
                     Type.size_t);

       

       
       
       if (OS.Constants.libc._DARWIN_FEATURE_64_BIT_INODE) {
         
         SysFile.stat =
           declareFFI("stat$INODE64", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                         Type.path,
                          Type.stat.out_ptr
                     );
         SysFile.lstat =
           declareFFI("lstat$INODE64", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                         Type.path,
                          Type.stat.out_ptr
                     );
         SysFile.fstat =
           declareFFI("fstat$INODE64", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                         Type.fd,
                          Type.stat.out_ptr
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
                          Type.negativeone_or_nothing,
                       Type.int,
                            Type.path,
                             Type.stat.out_ptr);
         let lxstat =
           declareFFI(lxstat_name, ctypes.default_abi,
                          Type.negativeone_or_nothing,
                       Type.int,
                            Type.path,
                             Type.stat.out_ptr);
         let fxstat =
           declareFFI(fxstat_name, ctypes.default_abi,
                          Type.negativeone_or_nothing,
                       Type.int,
                              Type.fd,
                             Type.stat.out_ptr);

         SysFile.stat = function stat(path, buf) {
           return xstat(ver, path, buf);
         };
         SysFile.lstat = function stat(path, buf) {
           return lxstat(ver, path, buf);
         };
         SysFile.fstat = function stat(fd, buf) {
           return fxstat(ver, fd, buf);
         };
       } else {
         
         SysFile.stat =
           declareFFI("stat", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                         Type.path,
                          Type.stat.out_ptr
                     );
         SysFile.lstat =
           declareFFI("lstat", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                         Type.path,
                          Type.stat.out_ptr
                     );
         SysFile.fstat =
           declareFFI("fstat", ctypes.default_abi,
                       Type.negativeone_or_nothing,
                           Type.fd,
                          Type.stat.out_ptr
                     );
       }

       
       

       let _pipe =
         declareFFI("pipe", ctypes.default_abi,
            Type.negativeone_or_nothing,
               new SharedAll.Type("two file descriptors",
             ctypes.ArrayType(ctypes.int, 2)));

       
       let _pipebuf = new (ctypes.ArrayType(ctypes.int, 2))();

       SysFile.pipe = function pipe(array) {
         let result = _pipe(_pipebuf);
         if (result == -1) {
           return result;
         }
         array[0] = ctypes.CDataFinalizer(_pipebuf[0], _close);
         array[1] = ctypes.CDataFinalizer(_pipebuf[1], _close);
         return result;
       };
     };

     exports.OS.Unix = {
       File: {
         _init: init
       }
     };
   })(this);
}
