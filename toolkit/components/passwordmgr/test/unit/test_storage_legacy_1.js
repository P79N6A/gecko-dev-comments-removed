








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

var storage = Cc["@mozilla.org/login-manager/storage/legacy;1"].
              createInstance(Ci.nsILoginManagerStorage);
if (!storage)
    throw "Couldn't create storage instance.";



testnum++;
testdesc = "[ensuring file doesn't exist]";

var filename="this-file-does-not-exist-"+Math.floor(Math.random() * 10000);
var file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
file.initWithPath(OUTDIR);
file.append(filename);
var exists = file.exists();
if (exists) {
    
    
    file.remove(false);
    do_check_false(exists); 
}

testdesc = "Initialize with a non-existant data file";

LoginTest.initStorage(storage, OUTDIR, filename);

LoginTest.checkStorageData(storage, [], []);

if (file.exists())
    file.remove(false);


testnum++;
testdesc = "Initialize with signons-00.txt (a zero-length file)";

LoginTest.initStorage(storage, INDIR, "signons-00.txt",
                     null, null, /invalid file header/);
LoginTest.checkStorageData(storage, [], []);



testnum++;
testdesc = "Initialize with signons-01.txt (bad file header)";

LoginTest.initStorage(storage, INDIR, "signons-01.txt",
                     null, null, /invalid file header/);
LoginTest.checkStorageData(storage, [], []);



testnum++;
testdesc = "Initialize with signons-02.txt (valid, but empty)";

LoginTest.initStorage(storage, INDIR, "signons-02.txt");
LoginTest.checkStorageData(storage, [], []);



testnum++;
testdesc = "Initialize with signons-03.txt (1 disabled, 0 logins)";

LoginTest.initStorage(storage, INDIR, "signons-03.txt");
LoginTest.checkStorageData(storage, ["http://www.disabled.com"], []);



testnum++;
testdesc = "Initialize with signons-04.txt (1 disabled, 0 logins, extra '.')";




LoginTest.initStorage(storage, INDIR, "signons-04.txt");
LoginTest.checkStorageData(storage, ["http://www.disabled.com"], []);



testnum++;
testdesc = "Initialize with signons-05.txt (0 disabled, 1 login)";

LoginTest.initStorage(storage, INDIR, "signons-05.txt");
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



testnum++;
testdesc = "Initialize with signons-06.txt (1 disabled, 1 login)";

LoginTest.initStorage(storage, INDIR, "signons-06.txt");
LoginTest.checkStorageData(storage, ["https://www.site.net"], [testuser1]);



testnum++;
testdesc = "Initialize with signons-07.txt (0 disabled, 2 logins on same host)";

LoginTest.initStorage(storage, INDIR, "signons-07.txt");
LoginTest.checkStorageData(storage, [], [testuser1, testuser2]);

do_check_eq(2, storage.countLogins("http://dummyhost.mozilla.org", "", null));

do_check_eq(2, storage.countLogins("http://dummyhost.mozilla.org", "foo", null));

do_check_eq(0, storage.countLogins("http://dummyhost.mozilla.org", null, ""));

do_check_eq(0, storage.countLogins("blah", "", ""));



testnum++;
testdesc = "Initialize with signons-08.txt (500 disabled, 500 logins)";

LoginTest.initStorage(storage, INDIR, "signons-08.txt");

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



testnum++;
testdesc = "Initialize with signons-2c-01.txt";





LoginTest.initStorage(storage, INDIR, "signons-2c-01.txt");
LoginTest.checkStorageData(storage, [], [testuser1]);


testdesc = "Initialize with signons-2c-02.txt";
var testuser3 = new nsLoginInfo;
testuser3.init("http://dummyhost.mozilla.org", null, "Some Realm",
    "dummydude", "itsasecret", "", "");
LoginTest.initStorage(storage, INDIR, "signons-2c-02.txt");
LoginTest.checkStorageData(storage, [], [testuser3]);



testnum++;
testdesc = "Initialize with signons-2c-03.txt";


testuser3.init("http://dummyhost.mozilla.org", null,
               "http://dummyhost.mozilla.org",
               "dummydude", "itsasecret", "", "");
LoginTest.initStorage(storage, INDIR, "signons-2c-03.txt");
LoginTest.checkStorageData(storage, [], [testuser3]);


testnum++;
testdesc = "Initialize with signons-2d-01.txt";





LoginTest.initStorage(storage, INDIR, "signons-2d-01.txt");
LoginTest.checkStorageData(storage, [], [testuser1]);


testdesc = "Initialize with signons-2d-02.txt";
testuser3.init("http://dummyhost.mozilla.org", null, "Some Realm",
    "dummydude", "itsasecret", "", "");
LoginTest.initStorage(storage, INDIR, "signons-2d-02.txt");
LoginTest.checkStorageData(storage, [], [testuser3]);



testnum++;
testdesc = "Initialize with signons-2d-03.txt";





testuser1.init("http://dummyhost80.mozilla.org", null, "Some Realm",
    "dummydude", "itsasecret", "", "");
testuser2.init("https://dummyhost443.mozilla.org", null, "Some Realm",
    "dummydude", "itsasecret", "", "");

LoginTest.initStorage(storage, INDIR, "signons-2d-03.txt");
LoginTest.checkStorageData(storage, [], [testuser1, testuser2]);



testnum++;
testdesc = "Initialize with signons-2d-04.txt";





testuser1.init("http://dummyhost8080.mozilla.org:8080", null, "Some Realm",
    "dummydude", "itsasecret", "", "");
testuser2.init("https://dummyhost8080.mozilla.org:8080", null, "Some Realm",
    "dummydude", "itsasecret", "", "");

LoginTest.initStorage(storage, INDIR, "signons-2d-04.txt");
LoginTest.checkStorageData(storage, [], [testuser1, testuser2]);



testnum++;
testdesc = "Initialize with signons-2d-05.txt";




testuser1.init("http://dummyhost80.mozilla.org", null,
               "http://dummyhost80.mozilla.org",
               "dummydude", "itsasecret", "", "");
testuser2.init("https://dummyhost443.mozilla.org", null,
               "https://dummyhost443.mozilla.org",
               "dummydude", "itsasecret", "", "");

LoginTest.initStorage(storage, INDIR, "signons-2d-05.txt");
LoginTest.checkStorageData(storage, [], [testuser1, testuser2]);



testdesc = "Initialize with signons-2d-06.txt";
testuser1.init("http://dummyhost8080.mozilla.org:8080", null,
               "http://dummyhost8080.mozilla.org:8080",
               "dummydude", "itsasecret", "", "");
testuser2.init("https://dummyhost8080.mozilla.org:8080", null,
               "https://dummyhost8080.mozilla.org:8080",
               "dummydude", "itsasecret", "", "");

LoginTest.initStorage(storage, INDIR, "signons-2d-06.txt");
LoginTest.checkStorageData(storage, [], [testuser1, testuser2]);



testnum++;
testdesc = "Initialize with signons-2d-07.txt";




testuser1.init("http://dummyhost80.mozilla.org",
               "http://dummyhost80.mozilla.org", null,
               "dummydude", "itsasecret", "put_user_here", "put_pw_here");
testuser2.init("https://dummyhost443.mozilla.org",
               "https://dummyhost443.mozilla.org", null,
               "dummydude", "itsasecret", "put_user_here", "put_pw_here");

testuser3.init("http://dummyhost8080.mozilla.org:8080",
               "http://dummyhost8080.mozilla.org:8080", null,
               "dummydude", "itsasecret", "put_user_here", "put_pw_here");
LoginTest.initStorage(storage, INDIR, "signons-2d-07.txt");
LoginTest.checkStorageData(storage, [], [testuser1, testuser2, testuser3]);



testnum++;
testdesc = "Initialize with signons-2d-08.txt";




testuser1.init("ftp://protocol.mozilla.org", null,
               "ftp://protocol.mozilla.org",
               "dummydude", "itsasecret", "", "");
testuser2.init("ftp://form.mozilla.org", "", null,
               "dummydude", "itsasecret", "put_user_here", "put_pw_here");
testuser3.init("ftp://form2.mozilla.org", "http://cgi.mozilla.org", null,
               "dummydude", "itsasecret", "put_user_here", "put_pw_here");

LoginTest.initStorage(storage, INDIR, "signons-2d-08.txt");
LoginTest.checkStorageData(storage, [], [testuser1, testuser2, testuser3]);



testnum++;
testdesc = "Initialize with signons-2d-09.txt";






testuser1.init("ftp://protocol.mozilla.org", null, "ftp://protocol.mozilla.org",
               "urluser", "itsasecret", "", "");
testuser2.init("ftp://form.mozilla.org", "", null,
               "dummydude", "itsasecret", "put_user_here", "put_pw_here");
LoginTest.initStorage(storage, INDIR, "signons-2d-09.txt");
LoginTest.checkStorageData(storage, [], [testuser1, testuser2]);



testnum++;
testdesc = "Initialize with signons-2d-10.txt";



testuser1.init("eBay.companion.paypal.guard", "", null,
               "p", "paypalpass", "", "");
testuser2.init("eBay.companion.ebay.guard", "", null,
               "p", "ebaypass", "", "");
LoginTest.initStorage(storage, INDIR, "signons-2d-10.txt");
LoginTest.checkStorageData(storage, [], [testuser1, testuser2]);



testnum++;
testdesc = "Initialize with signons-06.txt (1 disabled, 1 login); test removeLogin";

testuser1.init("http://dummyhost.mozilla.org", "", null,
    "dummydude", "itsasecret", "put_user_here", "put_pw_here");
testuser2.init("http://dummyhost.mozilla.org", "", null,
    "dummydude2", "itsasecret2", "put_user2_here", "put_pw2_here");

LoginTest.initStorage(storage, INDIR, "signons-06.txt", OUTDIR, "signons-06-2.txt");
LoginTest.checkStorageData(storage, ["https://www.site.net"], [testuser1]);

testdesc = "test removeLogin";
storage.removeLogin(testuser1);
LoginTest.checkStorageData(storage, ["https://www.site.net"], []);



testnum++;
testdesc = "Initialize with signons-06.txt (1 disabled, 1 login); test modifyLogin";

LoginTest.initStorage(storage, INDIR, "signons-06.txt",  OUTDIR, "signons-06-3.txt");
LoginTest.checkStorageData(storage, ["https://www.site.net"], [testuser1]);

testdesc = "test modifyLogin";
storage.modifyLogin(testuser1, testuser2);
LoginTest.checkStorageData(storage, ["https://www.site.net"], [testuser2]);









testnum++;

testdesc = "checking import of JS formSubmitURL entries"

testuser1.init("http://jstest.site.org", "javascript:", null,
               "dummydude", "itsasecret", "put_user_here", "put_pw_here");
LoginTest.initStorage(storage, INDIR, "signons-427033-1.txt",
                               OUTDIR, "output-427033-1.txt");
LoginTest.checkStorageData(storage, [], [testuser1]);

testdesc = "[flush and reload for verification]"
LoginTest.initStorage(storage, OUTDIR, "output-427033-1.txt");
LoginTest.checkStorageData(storage, [], [testuser1]);



} catch (e) {
    throw "FAILED in test #" + testnum + " -- " + testdesc + ": " + e;
}
};
