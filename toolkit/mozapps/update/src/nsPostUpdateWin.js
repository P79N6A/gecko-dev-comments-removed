











































const URI_BRAND_PROPERTIES     = "chrome://branding/locale/brand.properties";

const KEY_APPDIR          = "XCurProcD";
const KEY_TMPDIR          = "TmpD";
const KEY_UPDROOT         = "UpdRootD";
const KEY_UAPPDATA        = "UAppData";


const PR_RDONLY      = 0x01;
const PR_WRONLY      = 0x02;
const PR_APPEND      = 0x10;

const PERMS_FILE     = 0644;
const PERMS_DIR      = 0700;

const nsIWindowsRegKey = Components.interfaces.nsIWindowsRegKey;

var gConsole = null;
var gAppUpdateLogPostUpdate = false;






function LOG(s) {
  if (gAppUpdateLogPostUpdate) {
    dump("*** PostUpdateWin: " + s + "\n");
    gConsole.logStringMessage(s);
  }
}




function getFile(key) {
  var dirSvc =
      Components.classes["@mozilla.org/file/directory_service;1"].
      getService(Components.interfaces.nsIProperties);
  return dirSvc.get(key, Components.interfaces.nsIFile);
}







function newFile(path) {
  var file = Components.classes["@mozilla.org/file/local;1"]
                       .createInstance(Components.interfaces.nsILocalFile);
  file.initWithPath(path);
  return file;
}




function openFileInputStream(file) {
  var stream =
      Components.classes["@mozilla.org/network/file-input-stream;1"].
      createInstance(Components.interfaces.nsIFileInputStream);
  stream.init(file, PR_RDONLY, 0, 0);
  return stream;
}




function openFileOutputStream(file, flags) {
  var stream =
      Components.classes["@mozilla.org/network/file-output-stream;1"].
      createInstance(Components.interfaces.nsIFileOutputStream);
  stream.init(file, flags, 0644, 0);
  return stream;
}



const PREFIX_FILE = "File: ";

function InstallLogWriter() {
}
InstallLogWriter.prototype = {
  _outputStream: null,  

  


  _writeLine: function(s) {
    s = s + "\r\n";
    this._outputStream.write(s, s.length);
  },

  



  _getUninstallLogFile: function() {
    var file = getFile(KEY_APPDIR); 
    file.append("uninstall");
    if (!file.exists())
      return null;

    file.append("uninstall.log");
    if (!file.exists())
      file.create(Components.interfaces.nsILocalFile.NORMAL_FILE_TYPE, PERMS_FILE);

    return file;
  },

  



  _getUpdateLogFile: function() {
    function appendUpdateLogPath(root) {
      var file = root.clone();
      file.append("updates");
      file.append("0");
      file.append("update.log");
      if (file.exists())
        return file;

      file = root; 
      file.append("updates");
      file.append("last-update.log");
      if (file.exists())
        return file;

      return null;
    }

    
    var file = null;
    var updRoot;
    try {
      updRoot = getFile(KEY_UPDROOT);
    } catch (e) {
    }
    if (updRoot) {
      file = appendUpdateLogPath(updRoot);

      
      
      if (!file)
        file = appendUpdateLogPath(getFile(KEY_UAPPDATA));
    }

    
    if (!file)
      file = appendUpdateLogPath(getFile(KEY_APPDIR));

    return file;
  },

  



  _readUpdateLog: function(logFile, entries) {
    var stream;
    try {
      stream = openFileInputStream(logFile).
          QueryInterface(Components.interfaces.nsILineInputStream);

      var line = {};
      while (stream.readLine(line)) {
        var data = line.value.split(" ");
        if (data[0] == "EXECUTE" && data[1] == "ADD") {
          
          
          var relPath = "\\" + data[2].replace(/\//g, "\\");
          entries[relPath] = null;
        }
      }
    } finally {
      if (stream)
        stream.close();
    }
  },

  



  _readXPInstallLog: function(logFile, entries) {
    var stream;
    try {
      stream = openFileInputStream(logFile).
          QueryInterface(Components.interfaces.nsILineInputStream);

      function fixPath(path, offset) {
        return path.substr(offset).replace(appDirPath, "");
      }

      var appDir = getFile(KEY_APPDIR);
      var appDirPath = appDir.path;
      var line = {};
      while (stream.readLine(line)) {
        var entry = line.value;
        
        
        var searchStr = "nstalling: ";
        var index = entry.indexOf(searchStr);
        if (index != -1) {
          entries[fixPath(entry, index + searchStr.length)] = null;
          continue;
        }

        searchStr = "Replacing: ";
        index = entry.indexOf(searchStr);
        if (index != -1) {
          entries[fixPath(entry, index + searchStr.length)] = null;
          continue;
        }

        searchStr = "Windows Shortcut: ";
        index = entry.indexOf(searchStr);
        if (index != -1) {
          entries[fixPath(entry + ".lnk", index + searchStr.length)] = null;
          continue;
        }
      }
    } finally {
      if (stream)
        stream.close();
    }
  },

  _readUninstallLog: function(logFile, entries) {
    var stream;
    try {
      stream = openFileInputStream(logFile).
          QueryInterface(Components.interfaces.nsILineInputStream);

      var line = {};
      var searchStr = "File: ";
      while (stream.readLine(line)) {
        var index = line.value.indexOf(searchStr);
        if (index != -1) {
          var str = line.value.substr(index + searchStr.length);
          entries.push(str);
        }
      }
    } finally {
      if (stream)
        stream.close();
    }
  },

  



  begin: function() {
    var updateLog = this._getUpdateLogFile();
    if (!updateLog)
      return;

    var newEntries = { };
    this._readUpdateLog(updateLog, newEntries);

    try {
      const nsIDirectoryEnumerator = Components.interfaces.nsIDirectoryEnumerator;
      const nsILocalFile = Components.interfaces.nsILocalFile;
      var prefixWizLog = "install_wizard";
      var uninstallDir = getFile(KEY_APPDIR); 
      uninstallDir.append("uninstall");
      var entries = uninstallDir.directoryEntries.QueryInterface(nsIDirectoryEnumerator);
      while (true) {
        var wizLog = entries.nextFile;
        if (!wizLog)
          break;
        if (wizLog instanceof nsILocalFile && !wizLog.isDirectory() &&
            wizLog.leafName.indexOf(prefixWizLog) == 0) {
          this._readXPInstallLog(wizLog, newEntries);
          wizLog.remove(false);
        }
      }
    }
    catch (e) {}
    if (entries)
      entries.close();

    var uninstallLog = this._getUninstallLogFile();
    var oldEntries = [];
    this._readUninstallLog(uninstallLog, oldEntries);

    
    for (var relPath in newEntries) {
      if (oldEntries.indexOf(relPath) != -1)
        delete newEntries[relPath];
    }

    if (newEntries.length == 0)
      return;

    
    
    
    
    
    gCopiedLog = getFile(KEY_TMPDIR);
    gCopiedLog.append("uninstall");
    gCopiedLog.createUnique(gCopiedLog.DIRECTORY_TYPE, PERMS_DIR);
    if (uninstallLog)
      uninstallLog.copyTo(gCopiedLog, "uninstall.log");
    gCopiedLog.append("uninstall.log");
    
    LOG("uninstallLog = " + uninstallLog.path);
    LOG("copiedLog = " + gCopiedLog.path);
    
    if (!gCopiedLog.exists())
      gCopiedLog.create(Components.interfaces.nsILocalFile.NORMAL_FILE_TYPE, 
                        PERMS_FILE);
      
    this._outputStream =
        openFileOutputStream(gCopiedLog, PR_WRONLY | PR_APPEND);

    
    
    
    for (var relPath in newEntries)
      this._writeLine(PREFIX_FILE + relPath);
  },

  end: function() {
    if (!this._outputStream)
      return;
    this._outputStream.close();
    this._outputStream = null;
  }
};

var installLogWriter;
var gCopiedLog;









function RegKey() {
  
  if (arguments.length == 3) {
    this._key = arguments[0];
    this._root = arguments[1];
    this._path = arguments[2];
  } else {
    this._key =
        Components.classes["@mozilla.org/windows-registry-key;1"].
        createInstance(nsIWindowsRegKey);
  }
}
RegKey.prototype = {
  _key: null,
  _root: null,
  _path: null,

  ACCESS_READ:  nsIWindowsRegKey.ACCESS_READ,

  ROOT_KEY_CURRENT_USER: nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
  ROOT_KEY_LOCAL_MACHINE: nsIWindowsRegKey.ROOT_KEY_LOCAL_MACHINE,
  ROOT_KEY_CLASSES_ROOT: nsIWindowsRegKey.ROOT_KEY_CLASSES_ROOT,
  
  close: function() {
    this._key.close();
    this._root = null;
    this._path = null;
  },

  open: function(rootKey, path, mode) {
    this._key.open(rootKey, path, mode);
    this._root = rootKey;
    this._path = path;
  },

  openChild: function(path, mode) {
    var child = this._key.openChild(path, mode);
    return new RegKey(child, this._root, this._path + "\\" + path);
  },

  readStringValue: function(name) {
    return this._key.readStringValue(name);
  },

  hasValue: function(name) {
    return this._key.hasValue(name);
  },

  hasChild: function(name) {
    return this._key.hasChild(name);
  },

  get childCount() {
    return this._key.childCount;
  },

  getChildName: function(index) {
    return this._key.getChildName(index);
  },

  toString: function() {
    var root;
    switch (this._root) {
    case this.ROOT_KEY_CLASSES_ROOT:
      root = "HKEY_KEY_CLASSES_ROOT";
      break;
    case this.ROOT_KEY_LOCAL_MACHINE:
      root = "HKEY_LOCAL_MACHINE";
      break;
    case this.ROOT_KEY_CURRENT_USER:
      root = "HKEY_CURRENT_USER";
      break;
    default:
      LOG("unknown root key");
      return "";
    }
    return root + "\\" + this._path;
  }
};





function haveOldInstall(key, brandFullName, version) {
  var ourInstallDir = getFile(KEY_APPDIR);
  var result = false;
  var childKey, productKey, mainKey;
  try {
    for (var i = 0; i < key.childCount; ++i) {
      var childName = key.getChildName(i);
      childKey = key.openChild(childName, key.ACCESS_READ);
      if (childKey.hasValue("CurrentVersion")) {
        for (var j = 0; j < childKey.childCount; ++j) {
          var productVer = childKey.getChildName(j); 
          productKey = childKey.openChild(productVer, key.ACCESS_READ);
          if (productKey.hasChild("Main")) {
            mainKey = productKey.openChild("Main", key.ACCESS_READ);
            var installDir = mainKey.readStringValue("Install Directory");
            mainKey.close();
            LOG("old install? " + installDir + " vs " + ourInstallDir.path);
            LOG("old install? " + childName + " vs " + brandFullName);
            LOG("old install? " + productVer.split(" ")[0] + " vs " + version);
            if (newFile(installDir).equals(ourInstallDir) &&
                (childName != brandFullName ||
                productVer.split(" ")[0] != version)) {
              result = true;
            }
          }
          productKey.close();
          if (result)
            break;
        }
      }
      childKey.close();
      if (result)
        break;
    }
  } catch (e) {
    result = false;
    if (childKey)
      childKey.close();
    if (productKey)
      productKey.close();
    if (mainKey)
      mainKey.close();
  }
  return result;
}

function checkRegistry()
{
  LOG("checkRegistry");

  var result = false;
  
  
  
  var app = Components.classes["@mozilla.org/xre/app-info;1"].
            getService(Components.interfaces.nsIXULAppInfo);
  if (app.name == "Firefox") {          
    try {
      var key = new RegKey();
      key.open(RegKey.prototype.ROOT_KEY_CLASSES_ROOT, "FirefoxHTML\\shell\\open\\command", key.ACCESS_READ);
      var commandKey = key.readStringValue("");
      LOG("commandKey = " + commandKey);
      
      result = (commandKey.indexOf("-requestPending") == -1);
    } catch (e) {
      LOG("failed to open command key for FirefoxHTML: " + e);
    }
    key.close();
  }
  return result;
}

function checkOldInstall(rootKey, vendorShortName, brandFullName, version)
{
  var key = new RegKey();
  var result = false;

  try {
    key.open(rootKey, "SOFTWARE\\" + vendorShortName, key.ACCESS_READ);
    LOG("checkOldInstall: " + key + " " + brandFullName + " " + version);
    result = haveOldInstall(key, brandFullName, version);
  } catch (e) {
    LOG("failed trying to find old install: " + e);
  }
  key.close();
  return result;
}



function nsPostUpdateWin() {
  gConsole = Components.classes["@mozilla.org/consoleservice;1"]
                       .getService(Components.interfaces.nsIConsoleService);
  var prefs = Components.classes["@mozilla.org/preferences-service;1"].
              getService(Components.interfaces.nsIPrefBranch);
  try {
    gAppUpdateLogPostUpdate = prefs.getBoolPref("app.update.log.all");
  }
  catch (ex) {
  }
  try {
    if (!gAppUpdateLogPostUpdate) 
      gAppUpdateLogPostUpdate = prefs.getBoolPref("app.update.log.PostUpdate");
  }
  catch (ex) {
  }
}

nsPostUpdateWin.prototype = {
  QueryInterface: function(iid) {
    if (iid.equals(Components.interfaces.nsIRunnable) ||
        iid.equals(Components.interfaces.nsISupports))
      return this;
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },

  run: function() {
    
    
    var updateUninstallFile = getFile(KEY_APPDIR); 
    updateUninstallFile.append("uninstall");
    updateUninstallFile.append("uninstall.update");
    if (updateUninstallFile.exists()) {
      LOG("nothing to do, uninstall.log has already been updated"); 
      return;
    }

    try {
      installLogWriter = new InstallLogWriter();
      try {
        installLogWriter.begin();
      } finally {
        installLogWriter.end();
        installLogWriter = null;
      }
    } catch (e) {
      LOG(e);
    } 
    
    var app =
      Components.classes["@mozilla.org/xre/app-info;1"].
        getService(Components.interfaces.nsIXULAppInfo).
        QueryInterface(Components.interfaces.nsIXULRuntime);

    var sbs =
      Components.classes["@mozilla.org/intl/stringbundle;1"].
      getService(Components.interfaces.nsIStringBundleService);
    var brandBundle = sbs.createBundle(URI_BRAND_PROPERTIES);

    var vendorShortName = brandBundle.GetStringFromName("vendorShortName");
    var brandFullName = brandBundle.GetStringFromName("brandFullName");

    if (!gCopiedLog && 
        !checkRegistry() &&
        !checkOldInstall(RegKey.prototype.ROOT_KEY_LOCAL_MACHINE, 
                         vendorShortName, brandFullName, app.version) &&
        !checkOldInstall(RegKey.prototype.ROOT_KEY_CURRENT_USER, 
                         vendorShortName, brandFullName, app.version)) {
      LOG("nothing to do, so don't launch the helper");
      return;
    }

    try {
      var winAppHelper = 
        app.QueryInterface(Components.interfaces.nsIWinAppHelper);

      
      if (gCopiedLog)
        LOG("calling postUpdate with: " + gCopiedLog.path);
      else
        LOG("calling postUpdate without a log");

      winAppHelper.postUpdate(gCopiedLog);
    } catch (e) {
      LOG("failed to launch the helper to do the post update cleanup: " + e); 
    }
  }
};



var gModule = {
  registerSelf: function(compMgr, fileSpec, location, type) {
    compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    
    for (var key in this._objects) {
      var obj = this._objects[key];
      compMgr.registerFactoryLocation(obj.CID, obj.className, obj.contractID,
                                      fileSpec, location, type);
    }
  },
  
  getClassObject: function(compMgr, cid, iid) {
    if (!iid.equals(Components.interfaces.nsIFactory))
      throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    for (var key in this._objects) {
      if (cid.equals(this._objects[key].CID))
        return this._objects[key].factory;
    }
    
    throw Components.results.NS_ERROR_NO_INTERFACE;
  },
  
  _makeFactory: #1= function(ctor) {
    function ci(outer, iid) {
      if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;
      return (new ctor()).QueryInterface(iid);
    } 
    return { createInstance: ci };
  },
  
  _objects: {
    manager: { CID        : Components.ID("{d15b970b-5472-40df-97e8-eb03a04baa82}"),
               contractID : "@mozilla.org/updates/post-update;1",
               className  : "nsPostUpdateWin",
               factory    : #1#(nsPostUpdateWin)
             },
  },
  
  canUnload: function(compMgr) {
    return true;
  }
};

function NSGetModule(compMgr, fileSpec) {
  return gModule;
}
