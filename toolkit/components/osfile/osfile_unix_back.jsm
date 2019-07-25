




















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
     let libc_candidates =  [ "libSystem.dylib",
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

       
       
       

       


       Types.mode_t = Object.create(
         Types.intn_t(OS.Constants.libc.OSFILE_SIZEOF_MODE_T),
         {name: {value: "mode_t"}});

       

       
       let _close =
         libc.declare("close", ctypes.default_abi,
                        ctypes.int,
                             ctypes.int);

       UnixFile.close = function close(fd) {
         
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

       UnixFile.rename =
         declareFFI("rename", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                        Types.string,
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

       

       
       

       let _pipe =
         declareFFI("pipe", ctypes.default_abi,
                     Types.negativeone_or_nothing,
                        Types.int.out_ptr);

       
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


       exports.OS.Unix.Path = {
         



         basename: function basename(path) {
           return path.slice(path.lastIndexOf("/") + 1);
         },
         







         dirname: function dirname(path) {
           let index = path.lastIndexOf("/");
           if (index == -1) {
             return ".";
           }
           while (index >= 0 && path[index] == "/") {
             --index;
           }
           return path.slice(0, index + 1);
         },
         










         join: function join(path ) {
           
           let paths = [];
           for each(let i in arguments) {
             if (i.length != 0 && i[0] == "/") {
               paths = [i];
             } else {
               paths.push(i);
             }
           }
           return paths.join("/");
         },
         


         normalize: function normalize(path) {
           let stack = [];
           let absolute;
           if (path.length >= 0 && path[0] == "/") {
             absolute = true;
           } else {
             absolute = false;
           }
           path.split("/").forEach(function loop(v) {
             switch (v) {
             case "":  case ".":
               break;
             case "..":
               if (stack.length == 0) {
                 if (absolute) {
                   throw new Error("Path is ill-formed: attempting to go past root");
                 } else {
                   stack.push("..");
                 }
               } else {
                 stack.pop();
               }
               break;
             default:
               stack.push(v);
             }
           });
           let string = stack.join("/");
           return absolute ? "/" + string : string;
         },
         










         split: function split(path) {
           return {
             absolute: path.length && path[0] == "/",
             components: path.split("/")
           };
         }
       };

       

       exports.OS.Unix.libc = libc;
       exports.OS.Unix.declareFFI = declareFFI;

     };
     exports.OS.Unix.File._init = init;
   })(this);
}
