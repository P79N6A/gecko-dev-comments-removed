

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
    var e, caughtError = false;

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
        caughtError = true;
        var err = e;
    }

    if (aExpectedError) {
        if (!caughtError)
            throw "Storage didn't throw as expected (" + aExpectedError + ")";

        if (!aExpectedError.test(err))
            throw "Storage threw (" + err + "), not (" + aExpectedError;

        
        dump("...that error was expected.\n\n");
    } else if (caughtError) {
        throw "Component threw unexpected error: " + err;
    }

    return;
  },


  




  checkStorageData : function (storage, ref_disabledHosts, ref_logins) {

    var stor_disabledHosts = storage.getAllDisabledHosts({});
    do_check_eq(ref_disabledHosts.length, stor_disabledHosts.length);
    
    var stor_logins = storage.getAllLogins({});
    do_check_eq(ref_logins.length, stor_logins.length);

    



    var i, j, found;
    for (i = 0; i < ref_disabledHosts.length; i++) {
        for (j = 0; !found && j < stor_disabledHosts.length; j++) {
            found = (ref_disabledHosts[i] == stor_disabledHosts[j]);
        }
        do_check_true(found || stor_disabledHosts.length == 0);
    }
    for (j = 0; j < stor_disabledHosts.length; j++) {
        for (i = 0; !found && i < ref_disabledHosts.length; i++) {
            found = (ref_disabledHosts[i] == stor_disabledHosts[j]);
        }
        do_check_true(found || stor_disabledHosts.length == 0);
    }

    



    var ref, stor;
    for (i = 0; i < ref_logins.length; i++) {
        for (j = 0; !found && j < stor_logins.length; j++) {
            found = ref_logins[i].equals(stor_logins[j]);
        }
        do_check_true(found || stor_logins.length == 0);
    }

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


var OUTDIR = profileDir.path;
var INDIR = do_get_file("toolkit/components/passwordmgr/test/unit/data/" +
                        "signons-00.txt").parent.path;




var keydb = do_get_file("toolkit/components/passwordmgr/test/unit/key3.db");
try {
    var oldfile = Cc["@mozilla.org/file/local;1"].
                  createInstance(Ci.nsILocalFile);
    oldfile.initWithPath(profileDir.path + "/key3.db");
    if (oldfile.exists())
        oldfile.remove(false);
} catch(e) { }

keydb.copyTo(profileDir, "key3.db");

