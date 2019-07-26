
























if (typeof Components != "undefined") {
  this.EXPORTED_SYMBOLS = ["OS"];
  let Scope = {};
  Components.utils.import("resource://gre/modules/Services.jsm", Scope);

  
  
  
  let syslib_necessary = true;
  try {
    syslib_necessary = Scope.Services.prefs.getBoolPref("toolkit.osfile.test.syslib_necessary");
  } catch (x) {
    
  }

  try {
    Components.utils.import("resource://gre/modules/osfile/osfile_win_allthreads.jsm", this);
  } catch (ex if !syslib_necessary && ex.message.startsWith("Could not open system library:")) {
    
  }
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
       if (path.startsWith("\\\\")) {
         
         let index = path.lastIndexOf("\\");
         if (index != 1) {
           return path.slice(index + 1);
         }
         return ""; 
       }
       return path.slice(Math.max(path.lastIndexOf("\\"),
                                  path.lastIndexOf(":")) + 1);
     },

     















     dirname: function dirname(path, options) {
       let noDrive = (options && options.winNoDrive);

       
       let index = path.lastIndexOf("\\");
       if (index == -1) {
         
         if (!noDrive) {
           
           return this.winGetDrive(path) || ".";
         } else {
           
           return ".";
         }
       }

       if (index == 1 && path.charAt(0) == "\\") {
         
         if (noDrive) {
           return ".";
         } else {
           return path;
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
           let component = trimBackslashes(subpath.slice(drive.length));
           if (component) {
             paths = [component];
           } else {
             paths = [];
           }
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
       if (path.startsWith("\\\\")) {
         
         if (path.length == 2) {
           return null;
         }
         let index = path.indexOf("\\", 2);
         if (index == -1) {
           return path;
         }
         return path.slice(0, index);
       }
       
       let index = path.indexOf(":");
       if (index <= 0) return null;
       return path.slice(0, index + 1);
     },

     





     winIsAbsolute: function winIsAbsolute(path) {
       let index = path.indexOf(":");
       return path.length > index + 1 && path[index + 1] == "\\";
     },

     



     normalize: function normalize(path) {
       let stack = [];

       
       let root = this.winGetDrive(path);
       if (root) {
         path = path.slice(root.length);
       }

       
       let absolute = this.winIsAbsolute(path);

       
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
             if (stack[stack.length - 1] == "..") {
               stack.push("..");
             } else {
               stack.pop();
             }
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

    



    let trimBackslashes = function trimBackslashes(string) {
      return string.replace(/^\\+|\\+$/g,'');
    };

   exports.OS.Path = exports.OS.Win.Path;
}(this));
