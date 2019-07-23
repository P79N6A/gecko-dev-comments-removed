

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


  



  initStorage : function (storage, aInputPathName,  aInputFileName,
                          aOutputPathName, aOutputFileName, aExpectedError) {
    var err = null;

    var inputFile  = Cc["@mozilla.org/file/local;1"]
                            .createInstance(Ci.nsILocalFile);
    inputFile.initWithPath(aInputPathName);
    inputFile.append(aInputFileName);

    var outputFile = null;
    if (aOutputFileName) {
        var outputFile = Cc["@mozilla.org/file/local;1"]
                                .createInstance(Ci.nsILocalFile);
        outputFile.initWithPath(aOutputPathName);
        outputFile.append(aOutputFileName);
    }

    try {
        storage.initWithFile(inputFile, outputFile);
    } catch (e) {
        err = e;
    }

    this.checkExpectedError(aExpectedError, err);

    return;
  },


  





  checkExpectedError : function (aExpectedError, aActualError) {
    if (aExpectedError) {
        if (!aActualError)
            throw "Storage didn't throw as expected (" + aExpectedError + ")";

        if (!aExpectedError.test(aActualError))
            throw "Storage threw (" + aActualError + "), not (" + aExpectedError;

        
        dump("...that error was expected.\n\n");
    } else if (aActualError) {
        throw "Component threw unexpected error: " + aActualError;
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
var OUTDIR = PROFDIR.path;
var INDIR = do_get_file("toolkit/components/passwordmgr/test/unit/data/" +
                        "signons-00.txt").parent.path;




var keydb = do_get_file("toolkit/components/passwordmgr/test/unit/key3.db");
try {
    var oldfile = profileDir.clone();
    oldfile.append("key3.db");
    if (oldfile.exists())
        oldfile.remove(false);
} catch(e) { }

keydb.copyTo(profileDir, "key3.db");

