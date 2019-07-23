

const NS_APP_USER_PROFILE_50_DIR = "ProfD";
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;


const LoginTest = {

  



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
    this.checkLogins(ref_logins, storage.getAllLogins());
    this.checkDisabledHosts(ref_disabledHosts, storage.getAllDisabledHosts());
  },

  




  checkLogins : function (expectedLogins, actualLogins) {
    do_check_eq(expectedLogins.length, actualLogins.length);
    for (let i = 0; i < expectedLogins.length; i++) {
        let found = false;
        for (let j = 0; !found && j < actualLogins.length; j++) {
            found = expectedLogins[i].equals(actualLogins[j]);
        }
        do_check_true(found);
    }
  },

  




  checkDisabledHosts : function (expectedHosts, actualHosts) {
    do_check_eq(expectedHosts.length, actualHosts.length);
    for (let i = 0; i < expectedHosts.length; i++) {
        let found = false;
        for (let j = 0; !found && j < actualHosts.length; j++) {
            found = (expectedHosts[i] == actualHosts[j]);
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


var PROFDIR = do_get_profile();
var DATADIR = do_get_file("data/");

var OUTDIR = PROFDIR.path;
var INDIR = DATADIR.path;



LoginTest.copyFile("key3.db");
