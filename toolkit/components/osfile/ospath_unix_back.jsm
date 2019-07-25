















if (typeof Components != "undefined") {
  var EXPORTED_SYMBOLS = ["OS"];
  Components.utils.import("resource://gre/modules/osfile/osfile_unix_allthreads.jsm");
}
(function(exports) {
   "use strict";
   if (!exports.OS) {
     exports.OS = {};
   }
   if (!exports.OS.Unix) {
     exports.OS.Unix = {};
   }
   if (exports.OS.Unix.Path) {
     return; 
   }
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
       exports.OS.Shared.LOG("normalize", "stack", stack.toSource());
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

   exports.OS.Path = exports.OS.Unix.Path;
}(this));
