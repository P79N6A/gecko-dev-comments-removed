








const STORAGE_TYPE = "mozStorage";

function run_test() {

try {

var testnum = 0;
var testdesc = "Setup of nsLoginInfo test-users";
var nsLoginInfo = new Components.Constructor(
                    "@mozilla.org/login-manager/loginInfo;1",
                    Components.interfaces.nsILoginInfo);
do_check_true(nsLoginInfo != null);

var testuser1 = new nsLoginInfo;
testuser1.init("http://dummyhost.mozilla.org", "", null,
    "dummydude", "itsasecret", "put_user_here", "put_pw_here");

var testuser2 = new nsLoginInfo;
testuser2.init("http://dummyhost.mozilla.org", "", null,
    "dummydude2", "itsasecret2", "put_user2_here", "put_pw2_here");



var testnum = 1;
var testdesc = "Initial connection to storage module"

var storage;
storage = LoginTest.initStorage(INDIR, "signons-empty.txt", OUTDIR, "signons-empty.sqlite");
storage.getAllLogins();

var testdesc = "[ensuring file exists]"
var file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
file.initWithPath(OUTDIR);
file.append("signons-empty.sqlite");
do_check_true(file.exists());

LoginTest.deleteFile(OUTDIR, "signons-empty.sqlite");


testnum++;
testdesc = "[ensuring file doesn't exist]";

var filename="this-file-does-not-exist.pwmgr.sqlite";
file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
file.initWithPath(OUTDIR);
file.append(filename);
if(file.exists())
    file.remove(false);

testdesc = "Initialize with a non-existant data file";

storage = LoginTest.reloadStorage(OUTDIR, filename);

LoginTest.checkStorageData(storage, [], []);

try {
    if (file.exists())
        file.remove(false);
} catch (e) { }



testnum++;
testdesc = "Initialize with signons-02.txt (valid, but empty)";

storage = LoginTest.initStorage(INDIR, "signons-02.txt", OUTDIR, "signons-02.sqlite");
LoginTest.checkStorageData(storage, [], []);

LoginTest.deleteFile(OUTDIR, "signons-02.sqlite");



testnum++;
testdesc = "Initialize with signons-03.txt (1 disabled, 0 logins)";

storage = LoginTest.initStorage(INDIR, "signons-03.txt", OUTDIR, "signons-03.sqlite");
LoginTest.checkStorageData(storage, ["http://www.disabled.com"], []);

LoginTest.deleteFile(OUTDIR, "signons-03.sqlite");



testnum++;
testdesc = "Initialize with signons-04.txt (1 disabled, 0 logins, extra '.')";




storage = LoginTest.initStorage(INDIR, "signons-04.txt", OUTDIR, "signons-04.sqlite");
LoginTest.checkStorageData(storage, ["http://www.disabled.com"], []);

LoginTest.deleteFile(OUTDIR, "signons-04.sqlite");



testnum++;
testdesc = "Initialize with signons-05.txt (0 disabled, 1 login)";

storage = LoginTest.initStorage(INDIR, "signons-05.txt", OUTDIR, "signons-05.sqlite");
LoginTest.checkStorageData(storage, [], [testuser1]);

do_check_eq(1, storage.countLogins("http://dummyhost.mozilla.org", "",    null));

do_check_eq(1, storage.countLogins("http://dummyhost.mozilla.org", "foo", null));

do_check_eq(0, storage.countLogins("http://dummyhost.mozilla.org", null,    ""));

do_check_eq(0, storage.countLogins("blah", "", ""));

do_check_eq(1, storage.countLogins("", "", null));

do_check_eq(1, storage.countLogins("", "foo", null));

do_check_eq(0, storage.countLogins(null, "", null));
do_check_eq(0, storage.countLogins(null, null, ""));
do_check_eq(0, storage.countLogins(null, "", ""));
do_check_eq(0, storage.countLogins(null, null, null));

LoginTest.deleteFile(OUTDIR, "signons-05.sqlite");



testnum++;
testdesc = "Initialize with signons-06.txt (1 disabled, 1 login)";

storage = LoginTest.initStorage(INDIR, "signons-06.txt", OUTDIR, "signons-06.sqlite");
LoginTest.checkStorageData(storage, ["https://www.site.net"], [testuser1]);

LoginTest.deleteFile(OUTDIR, "signons-06.sqlite");



testnum++;
testdesc = "Initialize with signons-07.txt (0 disabled, 2 logins on same host)";

storage = LoginTest.initStorage(INDIR, "signons-07.txt", OUTDIR, "signons-07.sqlite");
LoginTest.checkStorageData(storage, [], [testuser1, testuser2]);

do_check_eq(2, storage.countLogins("http://dummyhost.mozilla.org", "", null));

do_check_eq(2, storage.countLogins("http://dummyhost.mozilla.org", "foo", null));

do_check_eq(0, storage.countLogins("http://dummyhost.mozilla.org", null, ""));

do_check_eq(0, storage.countLogins("blah", "", ""));

LoginTest.deleteFile(OUTDIR, "signons-07.sqlite");



testnum++;
testdesc = "Initialize with signons-08.txt (500 disabled, 500 logins)";

storage = LoginTest.initStorage(INDIR, "signons-08.txt", OUTDIR, "signons-08.sqlite");

var disabledHosts = [];
for (var i = 1; i <= 500; i++) {
    disabledHosts.push("http://host-" + i + ".site.com");
}

var bulkLogin, logins = [];
for (i = 1; i <= 250; i++) {
    bulkLogin = new nsLoginInfo;
    bulkLogin.init("http://dummyhost.site.org", "http://cgi.site.org", null,
        "dummydude", "itsasecret", "usernameField-" + i, "passwordField-" + i);
    logins.push(bulkLogin);
}
for (i = 1; i <= 250; i++) {
    bulkLogin = new nsLoginInfo;
    bulkLogin.init("http://dummyhost-" + i + ".site.org", "http://cgi.site.org", null,
        "dummydude", "itsasecret", "usernameField", "passwordField");
    logins.push(bulkLogin);
}
LoginTest.checkStorageData(storage, disabledHosts, logins);


do_check_eq(250, storage.countLogins("http://dummyhost.site.org", "", ""));
do_check_eq(250, storage.countLogins("http://dummyhost.site.org", "", null));
do_check_eq(0,   storage.countLogins("http://dummyhost.site.org", null, ""));

do_check_eq(1, storage.countLogins("http://dummyhost-1.site.org", "", ""));
do_check_eq(1, storage.countLogins("http://dummyhost-1.site.org", "", null));
do_check_eq(0, storage.countLogins("http://dummyhost-1.site.org", null, ""));

do_check_eq(500, storage.countLogins("", "", ""));
do_check_eq(500, storage.countLogins("", "http://cgi.site.org", ""));
do_check_eq(500, storage.countLogins("", "http://cgi.site.org", null));
do_check_eq(0,   storage.countLogins("", "blah", ""));
do_check_eq(0,   storage.countLogins("", "", "blah"));

do_check_eq(0, storage.countLogins(null, "", ""));
do_check_eq(0, storage.countLogins(null, "http://cgi.site.org", ""));
do_check_eq(0, storage.countLogins(null, "http://cgi.site.org", null));
do_check_eq(0, storage.countLogins(null, null, null));

LoginTest.deleteFile(OUTDIR, "signons-08.sqlite");



testnum++;
testdesc = "Initialize with signons-06.txt (1 disabled, 1 login); test removeLogin";

storage = LoginTest.initStorage(INDIR, "signons-06.txt", OUTDIR, "signons-06-2.sqlite");
LoginTest.checkStorageData(storage, ["https://www.site.net"], [testuser1]);

storage.removeLogin(testuser1);
LoginTest.checkStorageData(storage, ["https://www.site.net"], []);

LoginTest.deleteFile(OUTDIR, "signons-06-2.sqlite");



testnum++;
testdesc = "Initialize with signons-06.txt (1 disabled, 1 login); test modifyLogin";

storage = LoginTest.initStorage(INDIR, "signons-06.txt", OUTDIR, "signons-06-3.sqlite");
LoginTest.checkStorageData(storage, ["https://www.site.net"], [testuser1]);


var err = null;
try {
    storage.modifyLogin(testuser2, testuser1);
} catch (e) {
    err = e;
}
LoginTest.checkExpectedError(/No matching logins/, err);
LoginTest.checkStorageData(storage, ["https://www.site.net"], [testuser1]);


storage.modifyLogin(testuser1, testuser2);
LoginTest.checkStorageData(storage, ["https://www.site.net"], [testuser2]);

LoginTest.deleteFile(OUTDIR, "signons-06-3.sqlite");









testnum++;

testdesc = "checking import of JS formSubmitURL entries"

testuser1.init("http://jstest.site.org", "javascript:", null,
               "dummydude", "itsasecret", "put_user_here", "put_pw_here");
storage = LoginTest.initStorage(INDIR, "signons-427033-1.txt",
                               OUTDIR, "signons-427033-1.sqlite");
LoginTest.checkStorageData(storage, [], [testuser1]);

testdesc = "[flush and reload for verification]"
storage = LoginTest.reloadStorage(OUTDIR, "signons-427033-1.sqlite");
LoginTest.checkStorageData(storage, [], [testuser1]);

LoginTest.deleteFile(OUTDIR, "signons-427033-1.sqlite");









testnum++;

testdesc = "checking import of partially corrupted signons3.txt"

testuser1.init("http://dummyhost.mozilla.org", "", null,
               "dummydude", "itsasecret", "put_user_here", "put_pw_here");
storage = LoginTest.initStorage(INDIR, "signons-500822-1.txt",
                               OUTDIR, "signons-500822-1.sqlite");




let matchData = Cc["@mozilla.org/hash-property-bag;1"].createInstance(Ci.nsIWritablePropertyBag2);
matchData.setPropertyAsAString("hostname", "http://dummyhost.mozilla.org");
matchData.setPropertyAsAString("usernameField", "put_user_here");
logins = storage.searchLogins({}, matchData);
do_check_eq(1, logins.length, "should match 1 login");

LoginTest.deleteFile(OUTDIR, "signons-500822-1.sqlite");


} catch (e) {
    throw "FAILED in test #" + testnum + " -- " + testdesc + ": " + e;
}
};
