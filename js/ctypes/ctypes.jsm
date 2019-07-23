





































let EXPORTED_SYMBOLS = [ "ctypes" ];

const Cc = Components.classes;
const Ci = Components.interfaces;

let ctypes = {
  types: Ci.nsIForeignLibrary,

  open: function(name) {
    let library = Cc["@mozilla.org/jsctypes;1"]
                  .createInstance(Ci.nsIForeignLibrary);

    let file;
    if (name instanceof Ci.nsILocalFile) {
      file = name;
    } else {
      file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
      file.initWithPath(name);
    }

    library.open(file);
    return library;
  }
};

