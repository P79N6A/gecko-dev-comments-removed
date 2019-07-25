















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
}(this));
