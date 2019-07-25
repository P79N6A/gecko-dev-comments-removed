




















{
  if (typeof Components != "undefined") {
    
    
    
    

    throw new Error("osfile_unix_back.jsm cannot be used from the main thread yet");
  }
  importScripts("resource://gre/modules/osfile/osfile_shared.jsm");
  (function(exports) {
     "use strict";
     if (!exports.OS) {
       exports.OS = {};
     }
     if (!exports.OS.Unix) {
       exports.OS.Unix = {};
     }
     if (exports.OS.Unix.File) {
       return; 
     }
     exports.OS.Unix.File = {};

     let LOG = OS.Shared.LOG.bind(OS.Shared, "Unix");

     
     let libc;
     let libc_candidates =  [ "libsystem.B.dylib",
                              "libc.so.6",
                              "libc.so" ];
     for (let i = 0; i < libc_candidates.length; ++i) {
       try {
         libc = ctypes.open(libc_candidates[i]);
         break;
       } catch (x) {
         LOG("Could not open libc "+libc_candidates[i]);
       }
     }
     if (!libc) {
       throw new Error("Could not open any libc.");
     }

     




     let init = function init(aDeclareFFI) {
       let declareFFI;
       if (aDeclareFFI) {
         declareFFI = aDeclareFFI.bind(null, libc);
       } else {
         declareFFI = OS.Shared.declareFFI.bind(null, libc);
       }

       
       let OSUnix = exports.OS.Unix;
       let UnixFile = exports.OS.Unix.File;
       if (!exports.OS.Types) {
         exports.OS.Types = {};
       }
       let Type = exports.OS.Shared.Type;
       let Types = Type;

       
       
       

       


       Types.fd =
         new Type("fd",
                  ctypes.int,
                  function(fd_int) {
                    return ctypes.CDataFinalizer(fd_int, _close);
                  });

       



       Types.negativeone_or_fd =
         new Type("negativeone_or_fd",
                  ctypes.int,
                  function(fd_int, operation) {
                    if (fd_int == -1) {
                      return -1;
                    }
                    return ctypes.CDataFinalizer(fd_int, _close);
                  });

       



       Types.negativeone_or_nothing =
         new Type("negativeone_or_nothing",
                  ctypes.int);

       



       Types.negativeone_or_ssize_t =
         new Type("negativeone_or_ssize_t",
                  ctypes.ssize_t,
                  Type.ssize_t.convert_from_c);

       


       Types.null_or_string =
         new Type("null_or_string",
                  ctypes.char.ptr);

       Types.string =
         new Type("string",
                  ctypes.char.ptr);

       
       
       

       


       Types.mode_t =
         Types.intn_t(OS.Constants.libc.OSFILE_SIZEOF_MODE_T).withName("mode_t");

       


       Types.time_t =
         Types.intn_t(OS.Constants.libc.OSFILE_SIZEOF_TIME_T).withName("time_t");

       Types.DIR =
         new Type("DIR",
                  ctypes.StructType("DIR"));

       Types.null_or_DIR_ptr =
         new Type("null_or_DIR*",
                  Types.DIR.out_ptr.implementation,
                  function(dir, operation) {
                    if (dir == null || dir.isNull()) {
                      return null;
                    }
                    return ctypes.CDataFinalizer(dir, _close_dir);
                  });

       
       
       
       
       
       
       
       
       
       
       {
         let dirent = new OS.Shared.HollowStructure("dirent",
           OS.Constants.libc.OSFILE_SIZEOF_DIRENT);
         dirent.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_DIRENT_D_TYPE,
           "d_type", ctypes.uint8_t);
         dirent.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_DIRENT_D_NAME,
           "d_name", ctypes.ArrayType(ctypes.char, OS.Constants.libc.OSFILE_SIZEOF_DIRENT_D_NAME));

         
         Types.dirent = dirent.getType();
         LOG("dirent is: " + Types.dirent.implementation.toSource());
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
                          "st_uid", ctypes.int);
         stat.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_STAT_ST_GID,
                          "st_gid", ctypes.int);

         
         
         
         
         
         stat.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_STAT_ST_ATIME,
                          "st_atime", Types.time_t.implementation);
         stat.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_STAT_ST_MTIME,
                          "st_mtime", Types.time_t.implementation);
         stat.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_STAT_ST_CTIME,
                          "st_ctime", Types.time_t.implementation);

         stat.add_field_at(OS.Constants.libc.OSFILE_OFFSETOF_STAT_ST_SIZE,
                        "st_size", Types.size_t.implementation);
         Types.stat = stat.getType();
       }


       

       
       let _close =
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

       UnixFile.free =
         libc.declare("free", ctypes.default_abi,
                        ctypes.void_t,
                           ctypes.voidptr_t);

       
       UnixFile.access =
         declareFFI("access", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                       Types.string,
                       Types.int);

       UnixFile.chdir =
         declareFFI("chdir", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                       Types.string);

       UnixFile.chmod =
         declareFFI("chmod", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                       Types.string,
                       Types.mode_t);

       UnixFile.chown =
         declareFFI("chown", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                       Types.string,
                        Types.uid_t,
                        Types.gid_t);

       UnixFile.copyfile =
         declareFFI("copyfile", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                     Types.string,
                       Types.string,
                      Types.void_t.in_ptr, 
                      Types.uint32_t);

       UnixFile.dup =
         declareFFI("dup", ctypes.default_abi,
                     Types.negativeone_or_fd,
                         Types.fd);

       UnixFile.chdir =
         declareFFI("chdir", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                       Types.string);

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
                     Types.null_or_string,
                        Types.char.out_ptr,
                       Types.size_t);

       UnixFile.getwd =
         declareFFI("getwd", ctypes.default_abi,
                     Types.null_or_string,
                        Types.char.out_ptr);

       
       

       
       UnixFile.get_current_dir_name =
         declareFFI("get_current_dir_name", ctypes.default_abi,
                     Types.null_or_string.releaseWith(UnixFile.free));

       
       UnixFile.getwd_auto =
         declareFFI("getwd", ctypes.default_abi,
                     Types.null_or_string.releaseWith(UnixFile.free),
                        Types.void_t.in_ptr);

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
                       Types.string,
                      Types.uid_t,
                      Types.gid_t);

       UnixFile.link =
         declareFFI("link", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                     Types.string,
                       Types.string);

       UnixFile.lseek =
         declareFFI("lseek", ctypes.default_abi,
                     Types.off_t,
                         Types.fd,
                     Types.off_t,
                     Types.int);

       UnixFile.mkdir =
         declareFFI("mkdir", ctypes.default_abi,
                     Types.int,
                     Types.string,
                     Types.int);

       UnixFile.mkstemp =
         declareFFI("mkstemp", ctypes.default_abi,
                     Types.null_or_string,
                    Types.string);

       UnixFile.open =
         declareFFI("open", ctypes.default_abi,
                    Types.negativeone_or_fd,
                      Types.string,
                    Types.int,
                      Types.int);

       UnixFile.opendir =
         declareFFI("opendir", ctypes.default_abi,
                     Types.null_or_DIR_ptr,
                       Types.string);

       UnixFile.pread =
         declareFFI("pread", ctypes.default_abi,
                     Types.negativeone_or_ssize_t,
                         Types.fd,
                        Types.char.out_ptr,
                     Types.size_t,
                     Types.off_t);

       UnixFile.pwrite =
         declareFFI("pwrite", ctypes.default_abi,
                     Types.negativeone_or_ssize_t,
                         Types.fd,
                        Types.char.in_ptr,
                     Types.size_t,
                     Types.off_t);

       UnixFile.read =
         declareFFI("read", ctypes.default_abi,
                    Types.negativeone_or_ssize_t,
                        Types.fd,
                       Types.char.out_ptr,
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
                        Types.string,
                        Types.string);

       UnixFile.rmdir =
         declareFFI("rmdir", ctypes.default_abi,
                     Types.int,
                       Types.string);

       UnixFile.splice =
         declareFFI("splice", ctypes.default_abi,
                     Types.long,
                      Types.fd,
                     Types.off_t.in_ptr,
                     Types.fd,
                    Types.off_t.in_ptr,
                        Types.size_t,
                      Types.unsigned_int); 

       UnixFile.strerror =
         declareFFI("strerror", ctypes.default_abi,
                     Types.null_or_string,
                     Types.int);

       UnixFile.symlink =
         declareFFI("symlink", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                     Types.string,
                       Types.string);

       UnixFile.truncate =
         declareFFI("truncate", ctypes.default_abi,
                    Types.negativeone_or_nothing,
                      Types.string,
                     Types.off_t);

       UnixFile.unlink =
         declareFFI("unlink", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                       Types.string);

       UnixFile.write =
         declareFFI("write", ctypes.default_abi,
                     Types.negativeone_or_ssize_t,
                         Types.fd,
                        Types.char.in_ptr,
                     Types.size_t);

       

       
       
       if (OS.Constants.libc._DARWIN_FEATURE_64_BIT_INODE) {
         
         UnixFile.stat =
           declareFFI("stat$INODE64", ctypes.default_abi,
                       Types.negativeone_or_nothing,
                         Types.string,
                          Types.stat.out_ptr
                     );
         UnixFile.lstat =
           declareFFI("lstat$INODE64", ctypes.default_abi,
                       Types.negativeone_or_nothing,
                         Types.string,
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
         
         let xstat =
           declareFFI("__xstat", ctypes.default_abi,
                          Types.negativeone_or_nothing,
                       Types.int,
                            Types.string,
                             Types.stat.out_ptr);
         let lxstat =
           declareFFI("__lxstat", ctypes.default_abi,
                          Types.negativeone_or_nothing,
                       Types.int,
                            Types.string,
                             Types.stat.out_ptr);
         let fxstat =
           declareFFI("__fxstat", ctypes.default_abi,
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
                         Types.string,
                          Types.stat.out_ptr
                     );
         UnixFile.lstat =
           declareFFI("lstat", ctypes.default_abi,
                       Types.negativeone_or_nothing,
                         Types.string,
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
         libc.declare("pipe", ctypes.default_abi,
                     ctypes.int,
                        ctypes.ArrayType(ctypes.int, 2));

       
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

       

       exports.OS.Unix.libc = libc;
       exports.OS.Unix.declareFFI = declareFFI;

     };
     exports.OS.Unix.File._init = init;
   })(this);
}
