



(function(global) {
   const Cc = Components.classes;
   const Ci = Components.interfaces;
   const Cu = Components.utils;
   const Cr = Components.results;

   var exports = {};

   var dirsvc = Cc["@mozilla.org/file/directory_service;1"]
                .getService(Ci.nsIProperties);

   function MozFile(path) {
     var file = Cc['@mozilla.org/file/local;1']
                .createInstance(Ci.nsILocalFile);
     file.initWithPath(path);
     return {
       get directoryEntries() {
         try {
           return file.directoryEntries;
         } catch (e if e.result == Cr.NS_ERROR_FILE_NOT_FOUND) {
           throw new Error("path does not exist: " + file.path);
         }
       },
       __proto__: file
     };
   }

   exports.join = function join(base) {
     if (arguments.length < 2)
       throw new Error("need at least 2 args");
     base = MozFile(base);
     for (var i = 1; i < arguments.length; i++)
       base.append(arguments[i]);
     return base.path;
   };

   exports.dirname = function dirname(path) {
     return MozFile(path).parent.path;
   };

   exports.list = function list(path) {
     var entries = MozFile(path).directoryEntries;
     var entryNames = [];
     while(entries.hasMoreElements()) {
       var entry = entries.getNext();
       entry.QueryInterface(Ci.nsIFile);
       entryNames.push(entry.leafName);
     }
     return entryNames;
   };

   if (global.window) {
     
     
     global.File = exports;
   } else if (global.exports) {
     
     for (name in exports) {
       global.exports[name] = exports[name];
     }
   } else {
     
     global.EXPORTED_SYMBOLS = [];
     for (name in exports) {
       global.EXPORTED_SYMBOLS.push(name);
       global[name] = exports[name];
     }
   }
 })(this);
