

const NS_APP_USER_PROFILE_50_DIR = "ProfD";
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;




const LoginTest = {

  



  makeDirectoryService : function () {
    
    
    const provider = {
        getFile : function(prop, persistent) {
            persistent.value = true;
            if (prop == NS_APP_USER_PROFILE_50_DIR) {
                return dirSvc.get("CurProcD", Ci.nsIFile);
            }
            throw Cr.NS_ERROR_FAILURE;
        },

        QueryInterface : function(iid) {
            if (iid.equals(Ci.nsIDirectoryServiceProvider) ||
                iid.equals(Ci.nsISupports)) {
                return this;
            }
            throw Cr.NS_ERROR_NO_INTERFACE;
        }
    };

    dirSvc.QueryInterface(Ci.nsIDirectoryService).registerProvider(provider);
  },


  



  initStorage : function (aInputPathName,  aInputFileName,
                          aOutputPathName, aOutputFileName, aExpectedError) {
    var err = null;

    var newStorage = this.newStorage();

    var inputFile = null;
    if (aInputFileName) {
        var inputFile  = Cc["@mozilla.org/file/local;1"].
                         createInstance(Ci.nsILocalFile);
        inputFile.initWithPath(aInputPathName);
        inputFile.append(aInputFileName);
    }

    var outputFile = null;
    if (aOutputFileName) {
        var outputFile = Cc["@mozilla.org/file/local;1"].
                         createInstance(Ci.nsILocalFile);
        outputFile.initWithPath(aOutputPathName);
        outputFile.append(aOutputFileName);

        
        
        if (outputFile.exists())
            outputFile.remove(false);
    }

    try {
        newStorage.initWithFile(inputFile, outputFile);
    } catch (e) {
        err = e;
    }

    this.checkExpectedError(aExpectedError, err);

    return newStorage;
  },


  




  reloadStorage : function (aInputPathName, aInputFileName, aExpectedError) {
    var err = null;
    var newStorage = this.newStorage();

    var inputFile = null;
    if (aInputFileName) {
        var inputFile  = Cc["@mozilla.org/file/local;1"].
                         createInstance(Ci.nsILocalFile);
        inputFile.initWithPath(aInputPathName);
        inputFile.append(aInputFileName);
    }

    try {
        
        
        
        if (STORAGE_TYPE == "legacy")
            newStorage.initWithFile(inputFile, null);
        else if (STORAGE_TYPE == "mozStorage")
            newStorage.initWithFile(null, inputFile);
        else
            throw "Unknown STORAGE_TYPE";
    } catch (e) {
        err = e;
    }

    if (aExpectedError)
        this.checkExpectedError(aExpectedError, err);
    else
        do_check_true(err == null);

    return newStorage;
  },


  





  checkExpectedError : function (aExpectedError, aActualError) {
    if (aExpectedError) {
        if (!aActualError)
            throw "Test didn't throw as expected (" + aExpectedError + ")";

        if (!aExpectedError.test(aActualError))
            throw "Test threw (" + aActualError + "), not (" + aExpectedError;

        
        dump("...that error was expected.\n\n");
    } else if (aActualError) {
        throw "Test threw unexpected error: " + aActualError;
    }
  },


  




  checkStorageData : function (storage, ref_disabledHosts, ref_logins) {

    var stor_disabledHosts = storage.getAllDisabledHosts({});
    do_check_eq(ref_disabledHosts.length, stor_disabledHosts.length);
    
    var stor_logins = storage.getAllLogins({});
    do_check_eq(ref_logins.length, stor_logins.length);

    


    var i, j, found;
    for (i = 0; i < ref_disabledHosts.length; i++) {
        found = false;
        for (j = 0; !found && j < stor_disabledHosts.length; j++) {
            found = (ref_disabledHosts[i] == stor_disabledHosts[j]);
        }
        do_check_true(found);
    }

    


    var ref, stor;
    for (i = 0; i < ref_logins.length; i++) {
        found = false;
        for (j = 0; !found && j < stor_logins.length; j++) {
            found = ref_logins[i].equals(stor_logins[j]);
        }
        do_check_true(found);
    }

  },

  




  countLinesInFile : function (aPathName,  aFileName) {
    var inputFile  = Cc["@mozilla.org/file/local;1"].
                     createInstance(Ci.nsILocalFile);
    inputFile.initWithPath(aPathName);
    inputFile.append(aFileName);
    if (inputFile.fileSize == 0)
      return 0;

    var inputStream = Cc["@mozilla.org/network/file-input-stream;1"].
                      createInstance(Ci.nsIFileInputStream);
    
    inputStream.init(inputFile, 0x01, -1, null);
    var lineStream = inputStream.QueryInterface(Ci.nsILineInputStream);

    var line = { value : null };
    var lineCount = 1; 
    while (lineStream.readLine(line)) 
        lineCount++;

    return lineCount;
  },

  newStorage : function () {
    var ID;

    if (STORAGE_TYPE == "legacy")
        ID = "@mozilla.org/login-manager/storage/legacy;1"; 
    else if (STORAGE_TYPE == "mozStorage")
        ID = "@mozilla.org/login-manager/storage/mozStorage;1"; 
    else
        throw "Unknown STORAGE_TYPE";

    var storage = Cc[ID].
                  createInstance(Ci.nsILoginManagerStorage);
    if (!storage)
      throw "Couldn't create storage instance.";
    return storage;
  },

  openDB : function (filename) {
    
    var dbfile = PROFDIR.clone();
    dbfile.append(filename);

    var ss = Cc["@mozilla.org/storage/service;1"].
             getService(Ci.mozIStorageService);
    var dbConnection = ss.openDatabase(dbfile);

    return dbConnection;
  },

  deleteFile : function (pathname, filename) {
    var file = Cc["@mozilla.org/file/local;1"].
               createInstance(Ci.nsILocalFile);
    file.initWithPath(pathname);
    file.append(filename);
    
    
    
    try {
      if (file.exists())
        file.remove(false);
    } catch (e) {}
  },

  
  copyFile : function (filename) {
    var file = DATADIR.clone();
    file.append(filename);

    var profileFile = PROFDIR.clone();
    profileFile.append(filename);

    if (profileFile.exists())
        profileFile.remove(false);

    file.copyTo(PROFDIR, filename);
  }
};



var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);
try {
    var profileDir = dirSvc.get(NS_APP_USER_PROFILE_50_DIR, Ci.nsIFile);
} catch (e) { }

if (!profileDir) {
    LoginTest.makeDirectoryService();
    profileDir = dirSvc.get(NS_APP_USER_PROFILE_50_DIR, Ci.nsIFile);
}



var PROFDIR = profileDir;
var DATADIR = do_get_file("data/");

var OUTDIR = PROFDIR.path;
var INDIR = DATADIR.path;



LoginTest.copyFile("key3.db");
