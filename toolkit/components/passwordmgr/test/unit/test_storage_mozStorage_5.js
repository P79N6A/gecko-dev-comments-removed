








const STORAGE_TYPE = "mozStorage";

function run_test() {

try {

var storage, testnum = 0;



testnum++;
var testdesc = "Create nsILoginInfo instances for testing with"

var nsLoginInfo = new Components.Constructor(
                    "@mozilla.org/login-manager/loginInfo;1",
                    Components.interfaces.nsILoginInfo);
var testuser1 = new nsLoginInfo;
testuser1.init("http://dummyhost.mozilla.org", "", null,
    "dummydude", "itsasecret", "put_user_here", "put_pw_here");

LoginTest.deleteFile(OUTDIR, "signons.sqlite");











testnum++;
var testdesc = "Initialization, reinitialization, & importing"

var storage;
storage = LoginTest.initStorage(INDIR, "signons-00.txt", null, null, /Initialization failed/);




storage = LoginTest.initStorage(INDIR, "signons-06.txt");
LoginTest.checkStorageData(storage, ["https://www.site.net"], [testuser1]);

LoginTest.deleteFile(OUTDIR, "signons.sqlite");









testnum++;
var testdesc = "Corrupt database and backup"

var filename = "signons-c.sqlite";

var corruptDB = do_get_file("data/corruptDB.sqlite");

var cfile = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
cfile.initWithPath(OUTDIR);
cfile.append(filename);
if (cfile.exists())
    cfile.remove(false);

corruptDB.copyTo(PROFDIR, filename)


do_check_true(cfile.exists());


storage = LoginTest.reloadStorage(OUTDIR, filename, /Initialization failed/);


var buFile = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
buFile.initWithPath(OUTDIR);
buFile.append(filename + ".corrupt");
do_check_true(buFile.exists());


do_check_false(cfile.exists());


storage = LoginTest.reloadStorage(OUTDIR, filename);


storage.addLogin(testuser1);
LoginTest.checkStorageData(storage, [], [testuser1]);


var file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
file.initWithPath(OUTDIR);
file.append(filename);
do_check_true(file.exists());

LoginTest.deleteFile(OUTDIR, filename + ".corrupt");
LoginTest.deleteFile(OUTDIR, filename);



} catch (e) {
 throw ("FAILED in test #" + testnum + " -- " + testdesc + ": " + e);
}

};
