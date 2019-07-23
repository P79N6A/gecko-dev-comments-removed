








function run_test() {

try {


return;



var testnum = 0;
var testdesc = "Initial connection to storage module"

var storage = LoginTest.newMozStorage();
if (!storage)
throw "Couldn't create storage instance.";



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

var storage = LoginTest.newMozStorage();

LoginTest.initStorage(storage, INDIR, "signons-00.txt");
try {
    storage.getAllLogins({});
} catch (e) {
    var error = e;
}
LoginTest.checkExpectedError(/Initialization failed/, error);




LoginTest.initStorage(storage, INDIR, "signons-06.txt");
LoginTest.checkStorageData(storage, ["https://www.site.net"], [testuser1]);

LoginTest.deleteFile(OUTDIR, "signons.sqlite");









testnum++;
var testdesc = "Corrupt database and backup"

var filename = "signons-c.sqlite";

var corruptDB = do_get_file("toolkit/components/passwordmgr/test/unit/data/" +
                            "corruptDB.sqlite");

corruptDB.copyTo(PROFDIR, filename)


var cfile = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
cfile.initWithPath(OUTDIR);
cfile.append(filename);
do_check_true(cfile.exists());


var storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, null, null, OUTDIR, filename, null, true);
try {
    storage.getAllLogins({});
} catch (e) {
    var error = e;
}
LoginTest.checkExpectedError(/Initialization failed/, error);


var buFile = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
buFile.initWithPath(OUTDIR);
buFile.append(filename + ".corrupt");
do_check_true(buFile.exists());


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
