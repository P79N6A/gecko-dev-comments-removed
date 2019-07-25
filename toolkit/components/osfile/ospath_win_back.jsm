























if (typeof Components != "undefined") {
  var EXPORTED_SYMBOLS = ["OS"];
  Components.utils.import("resource://gre/modules/osfile/osfile_win_allthreads.jsm");
}
(function(exports) {
   "use strict";
   if (!exports.OS) {
     exports.OS = {};
   }
   if (!exports.OS.Win) {
     exports.OS.Win = {};
   }
   if (exports.OS.Win.Path) {
     return; 
   }
   exports.OS.Win.Path = {
     



     basename: function basename(path) {
       ensureNotUNC(path);
       return path.slice(Math.max(path.lastIndexOf("\\"),
         path.lastIndexOf(":")) + 1);
     },

     













     dirname: function dirname(path, options) {
       ensureNotUNC(path);
       let noDrive = (options && options.winNoDrive);

       
       let index = path.lastIndexOf("\\");
       if (index == -1) {
         
         if (!noDrive) {
           
           return this.winGetDrive(path) || ".";
         } else {
           
           return ".";
         }
       }

       
       while (index >= 0 && path[index] == "\\") {
         --index;
       }

       
       let start;
       if (noDrive) {
         start = (this.winGetDrive(path) || "").length;
       } else {
         start = 0;
       }
       return path.slice(start, index + 1);
     },

     










     join: function join(path ) {
       let paths = [];
       let root;
       let absolute = false;
       for each(let subpath in arguments) {
         let drive = this.winGetDrive(subpath);
         let abs   = this.winIsAbsolute(subpath);
         if (drive) {
           root = drive;
           paths = [trimBackslashes(subpath.slice(drive.length))];
           absolute = abs;
         } else if (abs) {
           paths = [trimBackslashes(subpath)];
           absolute = true;
         } else {
           paths.push(trimBackslashes(subpath));
         }
       }
       let result = "";
       if (root) {
         result += root;
       }
       if (absolute) {
         result += "\\";
       }
       result += paths.join("\\");
       return result;
     },

     



     winGetDrive: function winGetDrive(path) {
       ensureNotUNC(path);
       let index = path.indexOf(":");
       if (index <= 0) return null;
       return path.slice(0, index + 1);
     },

     





     winIsAbsolute: function winIsAbsolute(path) {
       ensureNotUNC(path);
       return this._winIsAbsolute(path);
     },
     



     _winIsAbsolute: function _winIsAbsolute(path) {
       let index = path.indexOf(":");
       return path.length > index + 1 && path[index + 1] == "\\";
     },

     



     normalize: function normalize(path) {
       let stack = [];

       
       let root = this.winGetDrive(path);
       if (root) {
         path = path.slice(root.length);
       }

       
       let absolute = this._winIsAbsolute(path);

       
       path = path.replace("/", "\\");

       
       
       path.split("\\").forEach(function loop(v) {
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

       
       let result = stack.join("\\");
       if (absolute) {
         result = "\\" + result;
       }
       if (root) {
         result = root + result;
       }
       return result;
     },

     











     split: function split(path) {
       return {
         absolute: this.winIsAbsolute(path),
         winDrive: this.winGetDrive(path),
         components: path.split("\\")
       };
     }
   };

    






    let ensureNotUNC = function ensureNotUNC(path) {
       if (!path) {
          throw new TypeError("Expecting a non-null path");
       }
       if (path.length >= 2 && path[0] == "\\" && path[1] == "\\") {
          throw new Error("Module Path cannot handle UNC-formatted paths yet: " + path);
       }
    };

    



    let trimBackslashes = function trimBackslashes(string) {
      return string.replace(/^\\+|\\+$/g,'');
    };

   exports.OS.Path = exports.OS.Win.Path;
}(this));
