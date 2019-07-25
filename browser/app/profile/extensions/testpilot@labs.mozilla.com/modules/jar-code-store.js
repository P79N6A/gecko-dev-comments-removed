




































function JarStore() {
  try {
  let baseDirName = "TestPilotExperimentFiles"; 
  this._baseDir = null;
  this._localOverrides = {}; 
  this._index = {}; 
  this._lastModified = {}; 
  this._init( baseDirName );
  } catch (e) {
    console.warn("Error instantiating jar store: " + e);
  }
}
JarStore.prototype = {

  _init: function( baseDirectory ) {
    let prefs = require("preferences-service");
    this._localOverrides = JSON.parse(
      prefs.get("extensions.testpilot.codeOverride", "{}"));

    let dir = Cc["@mozilla.org/file/directory_service;1"].
      getService(Ci.nsIProperties).get("ProfD", Ci.nsIFile);
    dir.append(baseDirectory);
    this._baseDir = dir;
    if( !this._baseDir.exists() || !this._baseDir.isDirectory() ) {
      
      console.info("Creating: " + this._baseDir.path + "\n");
      this._baseDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0777);
    } else {
      
      
      let jarFiles = this._baseDir.directoryEntries;
      while(jarFiles.hasMoreElements()) {
        let jarFile = jarFiles.getNext().QueryInterface(Ci.nsIFile);
        
        if (jarFile.leafName.indexOf(".jar") != jarFile.leafName.length - 4) {
          continue;
        }
        this._lastModified[jarFile.leafName] = jarFile.lastModifiedTime;
        this._indexJar(jarFile);
      }
    }
  },

  _indexJar: function(jarFile) {
    let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"]
                .createInstance(Ci.nsIZipReader);
    zipReader.open(jarFile); 
    let entries = zipReader.findEntries(null);
    while(entries.hasMore()) {
      
      let entry = entries.getNext();
      if (entry.indexOf(".js") == entry.length - 3) {
        
       let moduleName = entry.slice(0, entry.length - 3);
       this._index[moduleName] = jarFile.path;
      }
    }
    zipReader.close();
  },

  _verifyJar: function(jarFile, expectedHash) {
    
    
    
    console.info("Attempting to verify jarfile vs hash = " + expectedHash);
    let istream = Cc["@mozilla.org/network/file-input-stream;1"]
                        .createInstance(Ci.nsIFileInputStream);
    
    istream.init(jarFile, 0x01, 0444, 0);
    let ch = Cc["@mozilla.org/security/hash;1"]
                   .createInstance(Ci.nsICryptoHash);
    
    ch.init(ch.SHA256);
    
    const PR_UINT32_MAX = 0xffffffff;
    ch.updateFromStream(istream, PR_UINT32_MAX);
    
    let hash = ch.finish(false);

    
    function toHexString(charCode)
    {
      return ("0" + charCode.toString(16)).slice(-2);
    }

    
    let s = [toHexString(hash.charCodeAt(i)) for (i in hash)].join("");
    

    return (s == expectedHash);
  },

  saveJarFile: function( filename, rawData, expectedHash ) {
    console.info("Saving a JAR file as " + filename + " hash = " + expectedHash);
    
    let jarFile;
    try {
      jarFile = this._baseDir.clone();
      
      jarFile.append(filename.split("/").pop());

      
      if (jarFile.exists()) {
        jarFile.remove(false);
      }
      
      jarFile.create( Ci.nsIFile.NORMAL_FILE_TYPE, 0600);
      let stream = Cc["@mozilla.org/network/safe-file-output-stream;1"].
                      createInstance(Ci.nsIFileOutputStream);
      stream.init(jarFile, 0x04 | 0x08 | 0x20, 0600, 0); 
      stream.write(rawData, rawData.length);
      if (stream instanceof Ci.nsISafeOutputStream) {
        stream.finish();
      } else {
        stream.close();
      }
      
      
      if (this._verifyJar(jarFile, expectedHash)) {
        this._indexJar(jarFile);
        this._lastModified[jarFile.leafName] = jarFile.lastModifiedTime;
      } else {
        console.warn("Bad JAR file, doesn't match hash: " + expectedHash);
        jarFile.remove(false);
      }
    } catch(e) {
      console.warn("Error in saving jar file: " + e);
      
      if (jarFile.exists()) {
        jarFile.remove(false);
      }
    }
  },

  resolveModule: function(root, path) {
    
    if (root != null) {
      
    }
    
    let module;
    if (path.indexOf(".js") == path.length - 3) {
      module = path.slice(0, path.length - 3);
    } else {
      module = path;
    }
    if (this._index[module]) {
      let resolvedPath = this._index[module] + "!" + module + ".js";
      return resolvedPath;
    }
    return null;
    
  },

  getFile: function(path) {
    
    
    if (this._localOverrides[path]) {
      let code = this._localOverrides[path];
      return {contents: code};
    }
    let parts = path.split("!");
    let filePath = parts[0];
    let entryName = parts[1];
    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
    file.initWithPath(filePath);
    return this._readEntryFromJarFile(file, entryName);
  },

  _readEntryFromJarFile: function(jarFile, entryName) {
    
    
    let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"]
                .createInstance(Ci.nsIZipReader);
    zipReader.open(jarFile); 
    let rawStream = zipReader.getInputStream(entryName);
    let stream = Cc["@mozilla.org/scriptableinputstream;1"].
      createInstance(Ci.nsIScriptableInputStream);
    stream.init(rawStream);
    try {
      let data = new String();
      let chunk = {};
      do {
        chunk = stream.read(-1);
        data += chunk;
      } while (chunk.length > 0);
      return {contents: data};
    } catch(e) {
      console.warn("Error reading entry from jar file: " + e );
    }
    return null;
  },


  getFileModifiedDate: function(filename) {
    
    
    filename = filename.split("/").pop();
    if (this._lastModified[filename]) {
      return (this._lastModified[filename]);
    } else {
      return 0;
    }
  },

  listAllFiles: function() {
    
    let x;
    let list = [x for (x in this._index)];
    return list;
  },

  setLocalOverride: function(path, code) {
    let prefs = require("preferences-service");
    this._localOverrides[path] = code;
    prefs.set("extensions.testpilot.codeOverride",
              JSON.stringify(this._localOverrides));
  }
};

exports.JarStore = JarStore;