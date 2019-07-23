








function run_test() {


return;

try {



var testnum = 0;
var testdesc = "Initial connection to storage module"

var storage = LoginTest.newMozStorage();
if (!storage)
    throw "Couldn't create storage instance.";



testnum++;
var testdesc = "Create nsILoginInfo instances for testing with"

var dummyuser1 = Cc["@mozilla.org/login-manager/loginInfo;1"].
                 createInstance(Ci.nsILoginInfo);
var dummyuser2 = Cc["@mozilla.org/login-manager/loginInfo;1"].
                 createInstance(Ci.nsILoginInfo);
var dummyuser3 = Cc["@mozilla.org/login-manager/loginInfo;1"].
                 createInstance(Ci.nsILoginInfo);

dummyuser1.init("http://dummyhost.mozilla.org", "", null,
    "testuser1", "testpass1", "put_user_here", "put_pw_here");

dummyuser2.init("http://dummyhost2.mozilla.org", "", null,
    "testuser2", "testpass2", "put_user2_here", "put_pw2_here");

dummyuser3.init("http://dummyhost2.mozilla.org", "", null,
    "testuser3", "testpass3", "put_user3_here", "put_pw3_here");

LoginTest.deleteFile(OUTDIR, "signons.sqlite");










testnum++;

testdesc = "checking import of mime64-obscured entries"
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, INDIR, "signons-380961-1.txt",
                               OUTDIR, "output-380961-1.sqlite");
LoginTest.checkStorageData(storage, [], [dummyuser1]);

testdesc = "[flush and reload for verification]"
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, null, null, OUTDIR, "output-380961-1.sqlite");
LoginTest.checkStorageData(storage, [], [dummyuser1]);

LoginTest.deleteFile(OUTDIR, "output-380961-1.sqlite");



testnum++;

testdesc = "testing import of multiple mime-64 entries for a host"
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, INDIR, "signons-380961-2.txt",
                               OUTDIR, "output-380961-2.sqlite");
LoginTest.checkStorageData(storage, [], [dummyuser2, dummyuser3]);

testdesc = "[flush and reload for verification]"
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, null, null, OUTDIR, "output-380961-2.sqlite");
LoginTest.checkStorageData(storage, [], [dummyuser2, dummyuser3]);

LoginTest.deleteFile(OUTDIR, "output-380961-2.sqlite");



testnum++;

testdesc = "testing import of mixed encrypted and mime-64 entries."
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, INDIR, "signons-380961-3.txt",
                               OUTDIR, "output-380961-3.sqlite");
LoginTest.checkStorageData(storage, [], [dummyuser1, dummyuser2, dummyuser3]);

testdesc = "[flush and reload for verification]"
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, null, null, OUTDIR, "output-380961-3.sqlite");
LoginTest.checkStorageData(storage, [], [dummyuser1, dummyuser2, dummyuser3]);

LoginTest.deleteFile(OUTDIR, "output-380961-3.sqlite");












testnum++;

testdesc = "initializing login with non-ASCII data."
var dummyuser4 = Cc["@mozilla.org/login-manager/loginInfo;1"].
                 createInstance(Ci.nsILoginInfo);

dummyuser4.hostname      = "https://site.org";
dummyuser4.username      = String.fromCharCode(
                            355, 277, 349, 357, 533, 537, 101, 345, 185);
                            
dummyuser4.usernameField = "username";
dummyuser4.password      = "testpa" + String.fromCharCode(223) + "1";
                            
dummyuser4.passwordField = "password";
dummyuser4.formSubmitURL = "https://site.org";
dummyuser4.httpRealm     = null;



testnum++;

testdesc = "testing import of non-ascii username and password."
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, INDIR, "signons-381262.txt",
                               OUTDIR, "output-381262-1.sqlite");
LoginTest.checkStorageData(storage, [], [dummyuser4]);

testdesc = "[flush and reload for verification]"
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, null, null, OUTDIR, "output-381262-1.sqlite");
LoginTest.checkStorageData(storage, [], [dummyuser4]);

LoginTest.deleteFile(OUTDIR, "output-381262-1.sqlite");



testnum++;

testdesc = "testing storage of non-ascii username and password."
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, INDIR, "signons-empty.txt",
                               OUTDIR, "output-381262-2.sqlite");
LoginTest.checkStorageData(storage, [], []);
storage.addLogin(dummyuser4);
LoginTest.checkStorageData(storage, [], [dummyuser4]);

testdesc = "[flush and reload for verification]"
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, null, null, OUTDIR, "output-381262-2.sqlite");
LoginTest.checkStorageData(storage, [], [dummyuser4]);

LoginTest.deleteFile(OUTDIR, "output-381262-2.sqlite");










testnum++;

testdesc = "checking double reading of mime64-obscured entries";
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, INDIR, "signons-380961-1.txt",
                               OUTDIR, "output-400751-0.sqlite");
LoginTest.checkStorageData(storage, [], [dummyuser1]);

testdesc = "checking double reading of mime64-obscured entries part 2";
LoginTest.checkStorageData(storage, [], [dummyuser1]);

LoginTest.deleteFile(OUTDIR, "output-400751-0.sqlite");



testnum++;

testdesc = "checking correct storage of mime64 converted entries";
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, INDIR, "signons-380961-1.txt",
                               OUTDIR, "output-400751-1.sqlite");
LoginTest.checkStorageData(storage, [], [dummyuser1]);
LoginTest.checkStorageData(storage, [], [dummyuser1]);
storage.addLogin(dummyuser2); 
LoginTest.checkStorageData(storage, [], [dummyuser1, dummyuser2]);

testdesc = "[flush and reload for verification]";
LoginTest.initStorage(storage, null, null, OUTDIR, "output-400751-1.sqlite");
LoginTest.checkStorageData(storage, [], [dummyuser1, dummyuser2]);

LoginTest.deleteFile(OUTDIR, "output-400751-1.sqlite");









testnum++;

function tryAddUser(storage, aUser, aExpectedError) {
    var err = null;
    try {
        storage.addLogin(aUser);
    } catch (e) {
        err = e;
    }

    LoginTest.checkExpectedError(aExpectedError, err);
}

testdesc = "preparting to try logins with bogus values";
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, INDIR, "signons-empty.txt",
                               OUTDIR, "output-394610-1.sqlite");
LoginTest.checkStorageData(storage, [], []);


var failUser = Cc["@mozilla.org/login-manager/loginInfo;1"].
               createInstance(Ci.nsILoginInfo);

failUser.init("http://failure.site.org",
    "http://failure.site.org", null,
    "username", "password", "uname", "pword");


testdesc = "storing data values with embedded newlines."



var failHost = "http://new\nline.never.net";
var error = null;
try {
    storage.setLoginSavingEnabled(failHost, false);
} catch (e) {
    error = e;
}
LoginTest.checkExpectedError(/Invalid hostname/, error);


failHost = "http://new\rline.never.net";
error = null;
try {
    storage.setLoginSavingEnabled(failHost, false);
} catch (e) {
    error = e;
}
LoginTest.checkExpectedError(/Invalid hostname/, error);



failUser.hostname = "http://fail\nure.site.org";
tryAddUser(storage, failUser, /login values can't contain newlines/);

failUser.hostname = "http://fail\rure.site.org";
tryAddUser(storage, failUser, /login values can't contain newlines/);

failUser.hostname = "http://failure.site.org";



failUser.httpRealm = "http://fail\nure.site.org";
failUser.formSubmitURL = null;
tryAddUser(storage, failUser, /login values can't contain newlines/);

failUser.httpRealm = "http://fail\rure.site.org";
failUser.formSubmitURL = null;
tryAddUser(storage, failUser, /login values can't contain newlines/);

failUser.formSubmitURL = "http://fail\nure.site.org";
failUser.httpRealm = null;
tryAddUser(storage, failUser, /login values can't contain newlines/);

failUser.formSubmitURL = "http://fail\rure.site.org";
failUser.httpRealm = null;
tryAddUser(storage, failUser, /login values can't contain newlines/);

failUser.formSubmitURL = "http://failure.site.org";



failUser.usernameField = "u\nname";
tryAddUser(storage, failUser, /login values can't contain newlines/);

failUser.usernameField = "u\rname";
tryAddUser(storage, failUser, /login values can't contain newlines/);

failUser.usernameField = "uname";



failUser.passwordField = "p\nword";
tryAddUser(storage, failUser, /login values can't contain newlines/);

failUser.passwordField = "p\rword";
tryAddUser(storage, failUser, /login values can't contain newlines/);

failUser.passwordField = "pword";



failUser.username = "user\r\nname";
failUser.password = "pass\r\nword";
tryAddUser(storage, failUser, null);

testdesc = "[flush and reload for verification]"
LoginTest.initStorage(storage, OUTDIR, "output-394610-1.txt");
LoginTest.checkStorageData(storage, [], [failUser]);

failUser.username = "username";
failUser.password = "password";

LoginTest.deleteFile(OUTDIR, "output-394610-1.sqlite");



testnum++;

testdesc = "storing data values with special period-only value"
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, INDIR, "signons-empty.txt",
                               OUTDIR, "output-394610-2.sqlite");
LoginTest.checkStorageData(storage, [], []);


failHost = ".";
error = null;
try {
    storage.setLoginSavingEnabled(failHost, false);
} catch (e) {
    error = e;
}
LoginTest.checkExpectedError(/Invalid hostname/, error);




failUser.usernameField = ".";
tryAddUser(storage, failUser, /login values can't be periods/);
failUser.usernameField = "uname";


failUser.usernameField = ".";
tryAddUser(storage, failUser, /login values can't be periods/);
failUser.formSubmitURL = "http://failure.site.org";

testdesc = "check added data"
LoginTest.checkStorageData(storage, [], []);

testdesc = "[flush and reload for verification]"
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, null, null, OUTDIR, "output-394610-2.sqlite");
LoginTest.checkStorageData(storage, [], []);

LoginTest.deleteFile(OUTDIR, "output-394610-2.sqlite");



testnum++;

testdesc = "create logins with parens in host/httpRealm"

storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, INDIR, "signons-empty.txt",
                               OUTDIR, "output-394610-3.sqlite");
LoginTest.checkStorageData(storage, [], []);

var parenUser1 = Cc["@mozilla.org/login-manager/loginInfo;1"].
                 createInstance(Ci.nsILoginInfo);
var parenUser2 = Cc["@mozilla.org/login-manager/loginInfo;1"].
                 createInstance(Ci.nsILoginInfo);
var parenUser3 = Cc["@mozilla.org/login-manager/loginInfo;1"].
                 createInstance(Ci.nsILoginInfo);
var parenUser4 = Cc["@mozilla.org/login-manager/loginInfo;1"].
                 createInstance(Ci.nsILoginInfo);
var parenUser5 = Cc["@mozilla.org/login-manager/loginInfo;1"].
                 createInstance(Ci.nsILoginInfo);
var parenUser6 = Cc["@mozilla.org/login-manager/loginInfo;1"].
                 createInstance(Ci.nsILoginInfo);
var parenUser7 = Cc["@mozilla.org/login-manager/loginInfo;1"].
                 createInstance(Ci.nsILoginInfo);
var parenUser8 = Cc["@mozilla.org/login-manager/loginInfo;1"].
                 createInstance(Ci.nsILoginInfo);
var parenUser9 = Cc["@mozilla.org/login-manager/loginInfo;1"].
                 createInstance(Ci.nsILoginInfo);


parenUser1.init("http://parens.site.org", null, "(realm",
    "user1", "pass1", "uname", "pword");
parenUser2.init("http://parens.site.org", null, "realm)",
    "user2", "pass2", "uname", "pword");
parenUser3.init("http://parens.site.org", null, "(realm)",
    "user3", "pass3", "uname", "pword");
parenUser4.init("http://parens.site.org", null, ")realm(",
    "user4", "pass4", "uname", "pword");


parenUser5.init("http://parens(yay.site.org", null, "realm",
    "user5", "pass5", "uname", "pword");
parenUser6.init("http://parens)yay.site.org", null, "realm",
    "user6", "pass6", "uname", "pword");
parenUser7.init("http://parens(yay).site.org", null, "realm",
    "user7", "pass7", "uname", "pword");
parenUser8.init("http://parens)yay(.site.org", null, "realm",
    "user8", "pass8", "uname", "pword");


parenUser9.init("http://parens (.site.org", null, "realm",
    "user9", "pass9", "uname", "pword");

testdesc = "add logins with parens in host/httpRealm"
tryAddUser(storage, parenUser1, null);
tryAddUser(storage, parenUser2, null);
tryAddUser(storage, parenUser3, null);
tryAddUser(storage, parenUser4, null);
tryAddUser(storage, parenUser5, null);
tryAddUser(storage, parenUser6, null);
tryAddUser(storage, parenUser7, null);
tryAddUser(storage, parenUser8, null);


tryAddUser(storage, parenUser9, /bad parens in hostname/);

var parenLogins = [
    parenUser1, parenUser2, parenUser3, parenUser4,
    parenUser5, parenUser6, parenUser7, parenUser8
    ];

testdesc = "check added data"
LoginTest.checkStorageData(storage, [], parenLogins);

testdesc = "[flush and reload for verification]"
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, null, null, OUTDIR, "output-394610-3.sqlite");
LoginTest.checkStorageData(storage, [], parenLogins);

LoginTest.deleteFile(OUTDIR, "output-394610-3.sqlite");



testnum++;

testdesc = "storing data values with embedded nulls."


do_check_eq( "foo\0bar", "foo\0bar");
do_check_neq("foo\0bar", "foobar");

storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, INDIR, "signons-empty.txt",
                               OUTDIR, "output-394610-4.sqlite");
LoginTest.checkStorageData(storage, [], []);

var nullUser = Cc["@mozilla.org/login-manager/loginInfo;1"].
               createInstance(Ci.nsILoginInfo);

nullUser.init("http://null.site.org",
    "http://null.site.org", null,
    "username", "password", "usernull", "passnull");


var nullHost = "http://never\0X.sit.org";
error = null;
try {
    storage.setLoginSavingEnabled(nullHost, false);
} catch (e) {
    error = e;
}
LoginTest.checkExpectedError(/Invalid hostname/, error);



nullUser.hostname = "http://null\0X.site.org";
tryAddUser(storage, nullUser, /login values can't contain nulls/);
nullUser.hostname = "http://null.site.org";



nullUser.httpRealm = "http://null\0X.site.org";
nullUser.formSubmitURL = null;
tryAddUser(storage, nullUser, /login values can't contain nulls/);

nullUser.formSubmitURL = "http://null\0X.site.org";
nullUser.httpRealm = null;
tryAddUser(storage, nullUser, /login values can't contain nulls/);

nullUser.formSubmitURL = "http://null.site.org";



nullUser.usernameField = "usernull\0X";
tryAddUser(storage, nullUser, /login values can't contain nulls/);
nullUser.usernameField = "usernull";


nullUser.usernameField = ".\0";
tryAddUser(storage, nullUser, /login values can't contain nulls/);
nullUser.usernameField = "usernull";


nullUser.passwordField = "passnull\0X";
tryAddUser(storage, nullUser, /login values can't contain nulls/);
nullUser.passwordField = "passnull";



nullUser.username = "user\0name";
tryAddUser(storage, nullUser, /login values can't contain nulls/);
nullUser.username = "username";


nullUser.password = "pass\0word";
tryAddUser(storage, nullUser, /login values can't contain nulls/);
nullUser.password = "password";



LoginTest.checkStorageData(storage, [], []);

testdesc = "[flush and reload for verification]";
storage = LoginTest.newMozStorage();
LoginTest.initStorage(storage, null, null, OUTDIR, "output-394610-4.sqlite");
LoginTest.checkStorageData(storage, [], []);

nullUser.username = "username";
nullUser.password = "password";

LoginTest.deleteFile(OUTDIR, "output-394610-4.sqlite");



} catch (e) {
    throw ("FAILED in test #" + testnum + " -- " + testdesc + ": " + e);
}

};
